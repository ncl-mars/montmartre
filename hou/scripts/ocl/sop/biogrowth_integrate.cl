#include "$HIP/scripts/ocl/include/biogrowth.cl"
#include "$HIP/scripts/ocl/include/utils.cl"

// ---------------------------------------------------------------------------
// integrate — fused tip physics + interior vigor accumulation + light credit.
//
// Runs over every tree node.
//
// - All tree_nodes: accumulate/decay f@light_credit from the irradiance VDB.
// - Tips: shade-stressed early-return (branch autonomy) if below mortality,
//   else run the full physics pipeline (external forces → avoid → Euler →
//   allometric → collision → writeback).
// - Interiors: accumulate att_* into lateral_pressure for vigor-gated lateral
//   spawn downstream in topology.
//
// Both paths reset their accumulators so the node is clean next frame
// regardless of role. Shade-dead tips still reset accumulators before
// bailing — otherwise att_* compounds unboundedly across frozen frames.
// ---------------------------------------------------------------------------

#bind point  &P                 float3
#bind point   is_tip            int    read
#bind point  &velocity          float3
#bind point  &arc_since_spawn   float
#bind point  &att_ix            int
#bind point  &att_iy            int
#bind point  &att_iz            int
#bind point  &att_count         int
#bind point  &force_acc                float3
#bind point  &arc_total_since_lateral  float
#bind point  &lateral_pressure         float
#bind point  &last_att_force           float3
#bind point  &light_credit             float

#bind vdb    collision_sdf      float  input=1 read optional
#bind vdb    response_type      float  input=1 read optional
#bind vdb    irradiance         float  input=1 read optional

#bind parm damping          float  val=2.0
#bind parm max_speed        float  val=10.0
#bind parm tropism          float3 val={0.0, -1.0, 0.0}
#bind parm tropism_strength float  val=0.0


#bind parm collision_enable int val=1

#bind parm surface_band     float  val=0.0
#bind parm friction         float  val=0.3
#bind parm restitution      float  val=0.5

#bind point   branch_radius           float read

#bind parm allometric_enable   int   val=1
#bind parm allometric_k        float val=1.0
#bind parm allometric_exponent float val=0.6667
#bind parm allometric_curve    float val=2.0

#bind parm branch_autonomy_enable    int   val=0
#bind parm light_credit_decay        float val=0.05
#bind parm light_credit_mortality    float val=1.0
#bind parm light_credit_intake_gain  float val=1.0
#bind parm light_credit_curve        float val=2.0

#bind point  damage              float read
#bind parm   damage_enable       int   val=0
#bind parm   damage_intake_drain float val=1.0


@KERNEL
{
    // ----- light credit (all tree_nodes) -----
    // Accumulate irradiance intake with slow decay. Updates every frame,
    // independent of tip/interior role. Consumed as a mortality gate in
    // the tip branch below. No-op if irradiance VDB unwired.
    if (@branch_autonomy_enable > 0.5 && @irradiance.bound) {
        float intake = @irradiance.worldSample(@P) * @light_credit_intake_gain;
        float drain  = @damage * @damage_intake_drain * (float)(@damage_enable > 0);
        float c = @light_credit;
        c += (intake - drain) * @TimeInc;
        c *= (1.0f - @light_credit_decay * @TimeInc);
        @light_credit.set(c);
    }

    if (@is_tip > 0.5) {


        // ----- 1. external force contributors -----
        float3 att_force = (float3)(0.0f, 0.0f, 0.0f);
        if (@att_count > 0) {
            att_force = bg_decode_dir(@att_ix, @att_iy, @att_iz) / (float)@att_count;
        }
        float3 shared_force  = @force_acc;
        float3 tropism_force = bg_safe_normalize(@tropism) * @tropism_strength;

        float3 a = att_force + shared_force + tropism_force;
        float3 v = @velocity;


        // ----- 3. semi-implicit Euler -----
        float dt   = @TimeInc;
        float damp = max(0.0f, 1.0f - @damping * dt);
        v = (v + a * dt) * damp;
        v = limit(v, @max_speed);

        if (@allometric_enable > 0.5) {
            float L_max = @allometric_k * pow(@branch_radius, @allometric_exponent);
            if (L_max > 0.0f) {
                float ratio = clamp(@arc_total_since_lateral / L_max, 0.0f, 1.0f);
                float gf    = 1.0f - pow(ratio, @allometric_curve);
                v *= gf;
            }
        }

        // ----- branch autonomy: shade dims growth smoothly -----
        if (@branch_autonomy_enable > 0.5 && @irradiance.bound) {
            float denom = max(@light_credit_mortality, 1e-6f);
            float ratio = clamp(@light_credit / denom, 0.0f, 1.0f);
            // ratio=1 at/above mortality (healthy, full growth)
            // ratio=0 at zero credit         (shade-dead, frozen)
            float gf    = pow(ratio, @light_credit_curve);
            v *= gf;
        }

        // Damage growth-factor (independent of allometric)
        v *= max(1.0f - @damage * (float)(@damage_enable > 0), 0.0f);


        float3 dP     = v * dt;
        float  dP_len = length(dP);
        float3 P_new  = @P + dP;
        float  arc    = @arc_since_spawn + dP_len;

        // ----- 4. inline collision (hard post-step resolve) -----
        if (@collision_enable > 0.5 && @collision_sdf.bound) {
            float sdf_new = @collision_sdf.worldSample(P_new);
            if (sdf_new < @surface_band) {
                float3 n    = bg_safe_normalize(@collision_sdf.worldGradient(P_new));
                float  push = @surface_band - sdf_new;
                P_new += n * push;
                arc   -= push;

                int type = @response_type.bound
                        ? (int)round(@response_type.worldSample(P_new))
                        : 2;
                if (type != 0) {
                    float vn = dot(v, n);
                    if (vn < 0.0f) {
                        if      (type == 1) v = (float3)(0.0f, 0.0f, 0.0f);
                        else if (type == 2) v = (v - vn * n) * (1.0f - @friction);
                        else if (type == 3) v = (v - 2.0f * vn * n) * @restitution;
                    }
                }
            }
        }

        // ----- 5. writeback + reset -----
        @P.set(P_new);
        @velocity.set(v);
        @arc_since_spawn.set(arc);
        @arc_total_since_lateral.set(@arc_total_since_lateral + dP_len);

        @att_ix.set(0); @att_iy.set(0); @att_iz.set(0); 
        @att_count.set(0);
        @force_acc.set(0.0f);
    }

    else {
        if (@att_count <= 0) return;

        float3 f = bg_decode_dir(@att_ix, @att_iy, @att_iz);

        @lateral_pressure.set(@lateral_pressure + length(f) * @TimeInc);
        @last_att_force.set(f);

        @att_ix.set(0); @att_iy.set(0); @att_iz.set(0);
        // @att_count.set(0); reset in topology
    }
}
