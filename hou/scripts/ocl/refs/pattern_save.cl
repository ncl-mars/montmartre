#bind point &P float3
#bind point &Cd float3


int2 idToGid(int idx, float2 res){

    int2 xy = 0;
    xy.x = idx % (int)res.x;
    xy.y = floor(idx / res.x);
    return xy;
}


float2 gidToPos(int2 gid, float2 res){

    return ((convert_float2(gid)+0.5f)*2.0f - res) / max(res.x, res.y);
}


@KERNEL
{
    float3 pos = @P;
    float id = @elemnum;
    float2 res = (float2)(@g_resx, @g_resy);
    float2 dim = (float2)(@g_sizex, @g_sizey);

    int2 gid = idToGid(id, res);
    float2 gpos = gidToPos(gid, res);

    float2 spread = dim / (res-1.0f);

    pos.x += (gid.y % 2) * spread.x * 0.5;
    
    @P.set(pos);


    // float3 col = (float3)(gpos,0.0);
    // @Cd.set(col);
}





// ------------------------------------------------- bindings
    #bind point &P float3
    #bind point &class int
    #bind point &grid_ptnum int
    #bind point &Cd float3

    #bind point &is_column int

    float dummy(){return 0.0f;} // this line removes a weird vscode highlighting bug...

// ------------------------------------------------- constants
    #define DIV_PT_COUNT 5

    #ifndef PI
        #define PI 3.1415926535897932f
    #endif

// ------------------------------------------------- includes
    #include "matrix.h"
    #include "$HIP/scripts/ocl/include/hashs.cl"
    #include "$HIP/scripts/ocl/include/remapping.cl"

// ------------------------------------------------- types
    typedef struct{

        float l_thck;
        float k_off;
        float k_width;
        float k_sharp;
        float k_raise;
        float k_round;

    } pattern_t;

    typedef struct{

        int2 count;
        int2 res;
        float2 dim;

    } grid_t;

    typedef struct{

        int cur_id;
        int num;

    } point_t;
    
    typedef struct{

        float3 off;
        float3 col;

    } out_t; // tmp for debug purposes

// ------------------------------------------------- methods
    out_t rowPattern(point_t pt, grid_t grid, pattern_t pat){

        float3 off = 0.0f;

        int id_cur  = max(pt.num - pt.cur_id * grid.res.x,0);
        int dir     = (pt.cur_id % 2)*2-1; 

        float pt_spread = grid.dim.x / (float)(grid.res.x-1);
        float div_len   = DIV_PT_COUNT * pt_spread;

        int div_id = floor((id_cur+(DIV_PT_COUNT-1)*0.5) / DIV_PT_COUNT);
        int div_pt = div_id * DIV_PT_COUNT; 

        float diff = div_pt - id_cur;
        off.x = diff * pt_spread;

        float sgn = -sign(diff);
        diff = fabs(diff);

        if(diff > 0){
            float offx = div_len * pat.k_width * sgn; 
            if(diff > 1){
                offx += div_len * pat.k_sharp * sgn;
                off.z += -((div_id%2) * 2 - 1) * pat.k_round * dir; 
            }
            off.x += offx;
            off.y = offx * pat.k_raise * dir;
        }
        off.z += ((div_id % 2)*2-1)*(pat.k_off+pat.l_thck) * dir;

        out_t out = {off, hash31(div_id)}; 
        out.col = 1.0f;
    
        return out;
    }

    out_t colPattern(point_t pt, grid_t grid, pattern_t pat){
        
        float3 off = 0.0f;

        int r_count = grid.count.x * grid.res.x;

        int id_cur  = max( (pt.num-r_count) - (pt.cur_id - grid.count.x) * grid.res.y,0);
        int dir     = (pt.cur_id % 2)*2-1; 

        float pt_spread = grid.dim.y / (float)(grid.res.y-1);

        id_cur += (DIV_PT_COUNT-1)*dir -dir;
        int div_id = floor((id_cur+(DIV_PT_COUNT-1)) / (DIV_PT_COUNT*2.0f));

        
        int div_pt = div_id * DIV_PT_COUNT *2.0; 
        float diff = div_pt - id_cur;

        float div_len = DIV_PT_COUNT * pt_spread * 2.0f;

        float fac = fabs(diff)*pt_spread/div_len;

        fac = parabola(fac, 1.0);

        float k = 0.001;
        off.y = fac * k;
        off.y -= k*0.5;
        off.z += pt_spread*dir*0.5;


        out_t out = {off, hash31(div_id*100)}; 

        return out;
    }

// ------------------------------------------------- kernel
@KERNEL{

    point_t pt = { @class, @grid_ptnum};

    grid_t grid = {
        (int2)(@row_count, @col_count),
        (int2)(@row_points, @col_points),
        (float2)(@row_length, @col_length)
    };

    pattern_t pat = {
        @line_thick, 
        @knot_offset, 
        @knot_width_ratio, 
        1.0-@knot_sharp, 
        @knot_raise, 
        @knot_round
    };

    float3 pos = @P; 
    out_t res = {0.0f,1.0f};

    if(@is_column>0.5){
        res = colPattern(pt, grid, pat);
    }
    else{
        res = rowPattern(pt, grid, pat);
    }


    @P.set(pos + res.off);
    @Cd.set(res.col);
}




// ------------------------------------------------- bindings
    #bind point &P float3
    #bind point &class int
    #bind point &grid_ptnum int
    #bind point &Cd float3

    #bind point &is_column int

    float dummy(){return 0.0f;} // vscode weird highlight bug...

// ------------------------------------------------- constants
    #define DIV_PT_COUNT 5

    #ifndef PI
        #define PI 3.1415926535897932f
    #endif

// ------------------------------------------------- includes
    #include "matrix.h"
    #include "$HIP/scripts/ocl/include/hashs.cl"

// ------------------------------------------------- types
    typedef struct{

        float l_thck;
        float k_off;
        float k_width;
        float k_sharp;
        float k_raise;
        float k_round;

    } pattern_t;

    typedef struct{

        int2 res;
        float2 dim;

    } grid_t;

// ------------------------------------------------- methods

float3 rowPattern(grid_t grid, pattern_t pat){

    return 0.0f;
}


// ------------------------------------------------- kernel
@KERNEL{

    grid_t grid = {(int2)(@row_points, @col_points), (float2)(@row_length, @col_length)};

    pattern_t pat = {
        @line_thick, 
        @knot_offset, 
        @knot_width_ratio, 
        1.0-@knot_sharp, 
        @knot_raise, 
        @knot_round
    };


    //--
    float3 pos = @P;
    float3 off = 0.0f;

    int cur_id  = @class;

    int id_cur  = max(@grid_ptnum - cur_id * grid.res.x,0);
    int dir     = (cur_id % 2)*2-1; 

    float pt_spread = grid.dim.x / (float)(grid.res.x-1);
    float div_len   = DIV_PT_COUNT * pt_spread;

    int div_id = floor((id_cur+(DIV_PT_COUNT-1)*0.5) / DIV_PT_COUNT);
    int div_pt = div_id * DIV_PT_COUNT; 

    float diff = div_pt - id_cur;
    off.x = diff * pt_spread;

    float sgn = -sign(diff);
    diff = fabs(diff);

    if(diff > 0){
        float offx = div_len * pat.k_width * sgn; 
        if(diff > 1){
            offx += div_len * pat.k_sharp * sgn;
            off.z += -((div_id%2) * 2 - 1) * pat.k_round * dir; 
        }
        off.x += offx;
        off.y = offx * pat.k_raise * dir;
    }

    off.z += ((div_id % 2)*2-1)*(pat.k_off+pat.l_thck) * dir;
    
    @Cd.set( mix( hash31(div_id), (float3)(1.0f), @is_column) );
    
    off *= 1.0f-@is_column;


    pos += off;




    // off = 0.0f;
    // // off.x = dir * 0.0005 + 0.0009;
    // // if(diff > 0){ 1
    // off *= @is_column;
    // pos += off;


    @P.set(pos); 
}



////////////////////////////////////////////////////////// V0

// #include "matrix.h"
// #include "$HIP/scripts/ocl/include/hashs.cl"

// #define DIV_PT_COUNT 5

// #ifndef PI
//     #define PI 3.1415926535897932f
// #endif


// float3 pattern(float3 x, int octaves)
// {
//     return 0.0f;
// }


// //-- bindings
//     #bind point &P float3
//     #bind point &class int
//     #bind point &grid_ptnum int
//     #bind point &Cd float3

//     #bind parm knot_offset float
//     #bind parm knot_width_ratio float
//     #bind parm knot_sharp float
//     #bind parm knot_raise float
//     #bind parm knot_round float

//     #bind parm line_thick float

// //-- kernel
// @KERNEL
// {
//     float k_off     = @knot_offset;
//     float k_fac     = @knot_width_ratio;
//     float k_sharp   = 1.0-@knot_sharp;
//     float k_raise   = @knot_raise;
//     float k_round   = @knot_round;

//     int l_count     = @line_points;
//     float l_len     = @rest_length;
//     float l_thck    = @line_thick;


//     float3 pos = @P;
//     float3 off = 0.0f;

//     int cur_id  = @class;
//     int id_cur  = max(@grid_ptnum - cur_id * l_count,0);
//     int dir     = (cur_id % 2)*2-1; 

//     float pt_spread = l_len / (float)(l_count-1);
//     float div_len   = DIV_PT_COUNT * pt_spread;

//     int div_id = floor((id_cur+(DIV_PT_COUNT-1)*0.5) / DIV_PT_COUNT);
//     int div_pt = div_id * DIV_PT_COUNT; 

//     float diff = div_pt - id_cur;
//     off.x = diff * pt_spread;

//     float sgn = -sign(diff);
//     diff = fabs(diff);

//     if(diff > 0){
//         float offx = div_len * k_fac * sgn; 

//         if(diff > 1){
//             offx += div_len * k_sharp * sgn;
//             off.z += -((div_id%2) * 2 - 1) * k_round * dir; 
//         }
//         off.x += offx;
//         off.y = offx * k_raise * dir;
//     }

//     off.z += ((div_id % 2)*2-1)*(k_off+l_thck) * dir;

//     pos += off;

//     @Cd.set(hash31(div_id));
//     @P.set(pos); 
// }
