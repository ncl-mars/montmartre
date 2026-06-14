#include "$HIP/scripts/ocl/include/biogrowth.cl"

#bind point  parent_id  int   read
#bind point  &damage    float

#bind parm   damage_propagate_decay float val=0.95

@KERNEL
{
    if (@parent_id < 0) return;

    __global const float* dmg = (const __global float*)@damage.data;
    float parent_damage = dmg[@parent_id];

    float d = max(@damage, parent_damage * @damage_propagate_decay);
    @damage.set(clamp(d, 0.0f, 1.0f));
}
