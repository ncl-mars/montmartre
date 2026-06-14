#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  P             float3 read
#bind point  &damage       float

#bind vdb    damage_field  float input=1 read optional

#bind parm   damage_field_gain      float val=1.0
#bind parm   damage_field_threshold float val=0.0


@KERNEL
{
    if (!@damage_field.bound) return;          // no VDB wired → contributor inert

    float sample = @damage_field.worldSample(@P);
    if (sample <= @damage_field_threshold) return;

    float intake = (sample - @damage_field_threshold) * @damage_field_gain;
    float d      = @damage + intake * @TimeInc;
    @damage.set(clamp(d, 0.0f, 1.0f));
}
