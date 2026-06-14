#bind point &P float3

#bind point &Cd float3
#bind point uvpt  float2 input=1 read
#bind point N input=1 float3 read

#bind volume relief  float  input=2 resolution noforcealign


@KERNEL
{
    float2 uv = @uvpt;
    // uv = uv.yx;


    float2 pix = uv * (float2)(@relief.xres, @relief.yres);
    float relief = @relief.sample2d(pix);


    float3 pos = @P;
    float3 N = @N;
    float3 off = 0.0f;

    off = N * relief;
    pos += off;


    @P.set(pos);
    @Cd.set(1.0f);
}
