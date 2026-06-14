#include "$HIP/scripts/ocl/include/biogrowth.cl"

// ---------------------------------------------------------------------------
// murray_propagate — single-hop tips->root radius propagation.
// Each tree node pushes its branch_radius^p into its direct parent's
// murray_acc (int fixed-point atomic accumulator). Transitive tip->root
// propagation happens over successive frames as the tree grows —
// one hop per frame is enough to stay in sync with growth.
//
// Sibling children race-safe via atomic_add on int fixed-point.
// ---------------------------------------------------------------------------

#bind point  &murray_acc    int
#bind point   parent_id     int    read
#bind point   branch_radius float  read
#bind point   P             float3 read
#bind point   damage        float  read

#bind parm murray_exponent      float  val=2.0
#bind parm reaction_wood_enable int    val=0
#bind parm reaction_k           float  val=0.3
#bind parm up                   float3 val={0,1,0}
#bind parm damage_enable        int    val=0

@KERNEL
{
    if (@parent_id < 0) return;

    __global int*         acc  = (__global int*)@murray_acc.data;
    __global const float* Pbuf = (const __global float*)@P.data;

    float mult = 1.0f;
    if (@reaction_wood_enable > 0.5) {
        float3 parent_P = vload3(@parent_id, Pbuf);
        float3 axis     = @P - parent_P;
        float  len      = length(axis);
        float  downness = (len > 1e-6f) ? max(0.0f, -dot(axis / len, @up)) : 0.0f;
        mult = 1.0f + @reaction_k * downness;
    }

    float damage_factor = 1.0f - @damage * (float)(@damage_enable > 0);

    float v = pow(@branch_radius, @murray_exponent) * mult * damage_factor;
    int   q = (int)(v * BG_MURRAY_SCALE + 0.5f);
    atomic_add(&acc[@parent_id], q);
}
