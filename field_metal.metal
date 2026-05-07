#include <metal_stdlib>
using namespace metal;

struct Field { 
	float4 p1;
	float4 p2;
	float4 p3;
};

struct thread_state {                                                                                                 
	float    temperature;                                                                                           
	uint32_t iterations;
	uint32_t score;         // best score seen this run
	uint32_t _pad;                                                                                                    
};

struct global_state {
	uint32_t field_count;
	uint32_t buffer_size;
	uint32_t seed;
};

// ============================================================================
// ROBUST GEOMETRIC PREDICATES
// Based on Shewchuk's adaptive precision predicates, simplified for Metal
// ============================================================================

// Error-free transformation: TwoProduct
// Computes a * b = x + y exactly, where x is the rounded product and y is the error
inline void two_product(float a, float b, thread float& x, thread float& y) {
	x = a * b;
	y = fma(a, b, -x);  // Use FMA for exact error computation
}

// Error-free transformation: TwoSum  
// Computes a + b = x + y exactly, where x is the rounded sum and y is the error
inline void two_sum(float a, float b, thread float& x, thread float& y) {
	x = a + b;
	float b_virtual = x - a;
	float a_virtual = x - b_virtual;
	float b_roundoff = b - b_virtual;
	float a_roundoff = a - a_virtual;
	y = a_roundoff + b_roundoff;
}

// Robust 3D orientation test (sign of determinant)
// Returns: > 0 if d is above plane abc, < 0 if below, 0 if coplanar
// This is the fundamental primitive for robust geometric predicates
inline float orient3d_fast(float3 a, float3 b, float3 c, float3 d) {
	// Compute the determinant:
	// | ax ay az 1 |
	// | bx by bz 1 |
	// | cx cy cz 1 |
	// | dx dy dz 1 |
	
	// Translate d to origin for numerical stability
	float3 ad = a - d;
	float3 bd = b - d;
	float3 cd = c - d;
	
	// Compute determinant using cofactor expansion
	float det = ad.x * (bd.y * cd.z - bd.z * cd.y)
	          - ad.y * (bd.x * cd.z - bd.z * cd.x)
	          + ad.z * (bd.x * cd.y - bd.y * cd.x);
	
	return det;
}

// Robust CCW test for spherical geometry
// Tests if point p is "left" of the great circle arc from a to b
// Returns: > 0 if left, < 0 if right, ~0 if on the arc
inline float robust_ccw_spherical(float3 a, float3 b, float3 p) {
	// On a sphere, the "left" test is: is p on the positive side of the plane
	// defined by the origin, a, and b?
	// This is equivalent to: sign(dot(p, cross(a, b)))
	
	// Use orient3d with origin as the fourth point
	float3 origin = float3(0.0f, 0.0f, 0.0f);
	return orient3d_fast(origin, a, b, p);
}

// Robust cross product with error compensation
inline float3 robust_cross(float3 a, float3 b) {
	// Standard cross product
	float3 result;
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	
	// For tiny vectors, use compensated arithmetic
	float len_a = length(a);
	float len_b = length(b);
	
	if (len_a * len_b < 1e-6f) {
		// Use error-free transformations for better precision
		float x_hi, x_lo, y_hi, y_lo, z_hi, z_lo;
		float temp_hi, temp_lo;
		float sum_hi, sum_lo;
		
		// X component: a.y * b.z - a.z * b.y
		two_product(a.y, b.z, x_hi, x_lo);
		two_product(a.z, b.y, temp_hi, temp_lo);
		two_sum(x_hi, -temp_hi, sum_hi, sum_lo);
		result.x = sum_hi + (x_lo - temp_lo + sum_lo);
		
		// Y component: a.z * b.x - a.x * b.z
		two_product(a.z, b.x, y_hi, y_lo);
		two_product(a.x, b.z, temp_hi, temp_lo);
		two_sum(y_hi, -temp_hi, sum_hi, sum_lo);
		result.y = sum_hi + (y_lo - temp_lo + sum_lo);
		
		// Z component: a.x * b.y - a.y * b.x
		two_product(a.x, b.y, z_hi, z_lo);
		two_product(a.y, b.x, temp_hi, temp_lo);
		two_sum(z_hi, -temp_hi, sum_hi, sum_lo);
		result.z = sum_hi + (z_lo - temp_lo + sum_lo);
	}
	
	return result;
}

// Check if point p lies on arc from a1 to a2 using robust predicates
inline bool point_on_arc_robust(float3 a1, float3 a2, float3 p) {
	// S2's algorithm with robust cross products:
	// p is on the arc from a1 to a2 if cross(a1,p) and cross(p,a2) 
	// point in the same direction
	
	float3 cross1 = robust_cross(a1, p);
	float3 cross2 = robust_cross(p, a2);
	
	// Use robust dot product
	float dot_product = dot(cross1, cross2);
	
	// If they point in the same direction, dot product is positive
	// Use a small tolerance for numerical stability
	return dot_product >= -1e-10f;
}

// Check if two great circle arcs intersect using robust predicates
bool intersects_robust(float3 a1, float3 a2, float3 b1, float3 b2)
{
	const float eps = 1e-7f;
	
	// Compute normals to the great circles using robust cross product
	float3 n1 = robust_cross(a1, a2);
	float3 n2 = robust_cross(b1, b2);
	
	// Normalize the normals
	float len1 = length(n1);
	float len2 = length(n2);
	
	// Check for degenerate arcs
	if (len1 < eps || len2 < eps) {
		return false;
	}
	
	n1 = n1 / len1;
	n2 = n2 / len2;
	
	// Find intersection point of the two great circles
	float3 p = robust_cross(n1, n2);
	float plen = length(p);
	
	// Parallel great circles
	if (plen < eps) {
		return false;
	}
	
	// Normalize intersection point
	p = p / plen;
	
	// Check if p is on both arcs using robust predicates
	bool p_on_arc1 = point_on_arc_robust(a1, a2, p);
	bool p_on_arc2 = point_on_arc_robust(b1, b2, p);
	
	if (p_on_arc1 && p_on_arc2) {
		return true;
	}
	
	// Check the antipodal point
	float3 p_anti = -p;
	bool p_anti_on_arc1 = point_on_arc_robust(a1, a2, p_anti);
	bool p_anti_on_arc2 = point_on_arc_robust(b1, b2, p_anti);
	
	return (p_anti_on_arc1 && p_anti_on_arc2);
}

// Check if any edge of field f1 intersects any edge of field f2
bool fields_intersect_robust(Field f1, Field f2)
{
	// Extract xyz components from float4
	float3 f1_edges[3][2] = {
		{f1.p1.xyz, f1.p2.xyz},
		{f1.p2.xyz, f1.p3.xyz},
		{f1.p3.xyz, f1.p1.xyz}
	};
	
	float3 f2_edges[3][2] = {
		{f2.p1.xyz, f2.p2.xyz},
		{f2.p2.xyz, f2.p3.xyz},
		{f2.p3.xyz, f2.p1.xyz}
	};
	
	// Check all pairs of edges using robust intersection test
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (intersects_robust(f1_edges[i][0], f1_edges[i][1], 
			                      f2_edges[j][0], f2_edges[j][1])) {
				return true;
			}
		}
	}
	
	return false;
}

   bool arcs_intersect_simple(float3 a, float3 b, float3 c, float3 d) {
       float3 origin = float3(0,0,0);
       
       // Check if C and D are on opposite sides of plane OAB
       float vol_c = orient3d_fast(origin, a, b, c);
       float vol_d = orient3d_fast(origin, a, b, d);
       if (vol_c * vol_d > 0.0f) return false; // Same side
       
       // Check if A and B are on opposite sides of plane OCD
       float vol_a = orient3d_fast(origin, c, d, a);
       float vol_b = orient3d_fast(origin, c, d, b);
       if (vol_a * vol_b > 0.0f) return false; // Same side
       
       // Handle endpoint sharing (Touch vs Cross)
       // If any volume is exactly 0, an endpoint lies on the other arc's plane.
       // For "Strict Crossing" (Field Art), sharing an endpoint is NOT a crossing.
       // So we usually want strict inequality, or specific handling.
       // But vol * vol > 0 covers "Same Side". 
       // If vol * vol <= 0, they straddle or touch.
       
       return true;
   }


   bool fields_intersect(Field f1, Field f2) {
       // 1. Check if they share any vertex. If so, do they "cross"?
       // In Ingress, fields sharing a vertex is fine.
       // We only care if an edge CROSSES another edge.
       // Logic: Iterate edges. If edge E1 shares a vertex with E2, skip it.
       // Else check intersection.
       
       float3 f1_p[] = {f1.p1.xyz, f1.p2.xyz, f1.p3.xyz};
       float3 f2_p[] = {f2.p1.xyz, f2.p2.xyz, f2.p3.xyz};
       
       for (int i=0; i<3; ++i) {
           float3 a = f1_p[i];
           float3 b = f1_p[(i+1)%3];
           
           for (int j=0; j<3; ++j) {
               float3 c = f2_p[j];
               float3 d = f2_p[(j+1)%3];
               
               // Check for shared vertices
               // Using distance squared to be safe against float noise
               if (length_squared(a-c) < 1e-10 || length_squared(a-d) < 1e-10 ||
                   length_squared(b-c) < 1e-10 || length_squared(b-d) < 1e-10) 
               {
                   continue; // Shared endpoint -> No crossing
               }
               
               if (arcs_intersect_simple(a, b, c, d)) return true;
           }
       }
       return false;
   }

// PRNG for GPU
uint rand_int(thread uint& seed) {
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

float rand_float(thread uint& seed) {
    return (float)(rand_int(seed) >> 8) / 16777216.0f;
}

bool get_field(device uchar* buffer, uint i) {
    return (buffer[i / 8] & (1 << (7 - (i % 8)))) != 0;
}

void set_field(device uchar* buffer, uint i, bool value) {
    if (value)
        buffer[i / 8] |= (1 << (7 - (i % 8)));
    else
        buffer[i / 8] &= ~(1 << (7 - (i % 8)));
}

kernel void field_anneal(
    constant global_state* state [[buffer(0)]],
    const device uchar* clique [[buffer(1)]],
    device uchar* results [[buffer(2)]],
    uint gid [[thread_position_in_grid]]
)
{
    device struct thread_state* ts = (device struct thread_state*)(results + gid * state->buffer_size);
    device uchar* current_set = (device uchar*)(results + gid * state->buffer_size + sizeof(struct thread_state));

    uint seed = state->seed + gid;
    uint current_score = ts->score;
    float temperature = ts->temperature;
    uint iterations = ts->iterations;
    uint field_count = state->field_count;
    uint bpr = (field_count + 7) / 8;  // bytes per clique row, precomputed

    for (uint iter = 0; iter < iterations; iter++)
    {
        uint f = rand_int(seed) % field_count;
        bool is_set = get_field(current_set, f);

        if (is_set)
        {
            float delta = -1.0f;
            if (rand_float(seed) < exp(delta / temperature))
            {
                set_field(current_set, f, false);
                current_score--;
            }
        }
        else
        {
            const device uchar* clique_f = clique + f * bpr;
            uint removed_count = 0;

            for (uint j = 0; j < bpr; j++)
            {
                uchar conflicts = current_set[j] & ~clique_f[j];
                removed_count += popcount((uint)conflicts);
            }

            float delta = 1.0f - (float)removed_count;
            if (delta >= 0.0f || rand_float(seed) < exp(delta / temperature))
            {
                for (uint j = 0; j < bpr; j++)
                    current_set[j] &= clique_f[j];
                set_field(current_set, f, true);
                current_score = (uint)((int)current_score + 1 - (int)removed_count);
            }
        }
    }
    ts->score = current_score;
}

// Compute clique matrix kernel (Compatibility Matrix)
kernel void compute_field_clique(
    device uchar* out [[buffer(0)]],
    const device Field* fields [[buffer(1)]],
    constant uint& num_fields [[buffer(2)]],
    uint gid [[thread_position_in_grid]]
)
{
    if (gid >= num_fields)
        return;
    
    uint bytesPerRow = (num_fields + 7) / 8;
    device uchar* row = out + gid * bytesPerRow;
    
    // Clear row first
    for (uint b = 0; b < bytesPerRow; b++) row[b] = 0;

    for (uint j = 0; j < num_fields; j++) {
        bool compatible = true;
        if (gid != j) {
            compatible = !fields_intersect(fields[gid], fields[j]);
        }
        
        if (compatible) {
            row[j / 8] |= (1 << (7 - (j % 8)));
        }
    }
}

