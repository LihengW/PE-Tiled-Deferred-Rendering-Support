// compute_shader.glsl
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = vec4(float(pixel_coords.x) / 512.0, float(pixel_coords.y) / 512.0, 0.5, 1.0);
    imageStore(img_output, pixel_coords, color);
}