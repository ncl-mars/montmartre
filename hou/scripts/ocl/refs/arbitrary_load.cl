#bind point &P float3
#bind point &Cd float3
#bind point N input=1 float3 read

#bind parm test_idx int val=0

@KERNEL
{
    // Test: can we get a raw pointer to input 1's point attribute?
    __global const float* N_buf = (const __global float*)@N.data;

    // Read N from input 1 at arbitrary index (not the current point)
    float3 N_arbitrary = vload3(@test_idx, N_buf);

    // Remap [-1,1] -> [0,1] so it's visible as color
    @Cd.set(N_arbitrary * 0.5f + 0.5f);
}
