// Copyright 2021 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/DawnTest.h"

#include "utils/ComboRenderPipelineDescriptor.h"
#include "utils/WGPUHelpers.h"

#include <vector>

class ShaderTests : public DawnTest {};

// Test that log2 is being properly calculated, base on crbug.com/1046622
TEST_P(ShaderTests, ComputeLog2) {
    uint32_t const kSteps = 19;
    std::vector<uint32_t> data(kSteps, 0);
    std::vector<uint32_t> expected{0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 32};
    uint64_t bufferSize = static_cast<uint64_t>(data.size() * sizeof(uint32_t));
    wgpu::Buffer buffer = utils::CreateBufferFromData(
        device, data.data(), bufferSize, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc);

    std::string shader = R"(
[[block]] struct Buf {
    data : array<u32, 19>;
};

[[group(0), binding(0)]] var<storage> buf : [[access(read_write)]] Buf;

[[stage(compute)]] fn main() {
    let factor : f32 = 1.0001;

    buf.data[0] = u32(log2(1.0 * factor));
    buf.data[1] = u32(log2(2.0 * factor));
    buf.data[2] = u32(log2(3.0 * factor));
    buf.data[3] = u32(log2(4.0 * factor));
    buf.data[4] = u32(log2(7.0 * factor));
    buf.data[5] = u32(log2(8.0 * factor));
    buf.data[6] = u32(log2(15.0 * factor));
    buf.data[7] = u32(log2(16.0 * factor));
    buf.data[8] = u32(log2(31.0 * factor));
    buf.data[9] = u32(log2(32.0 * factor));
    buf.data[10] = u32(log2(63.0 * factor));
    buf.data[11] = u32(log2(64.0 * factor));
    buf.data[12] = u32(log2(127.0 * factor));
    buf.data[13] = u32(log2(128.0 * factor));
    buf.data[14] = u32(log2(255.0 * factor));
    buf.data[15] = u32(log2(256.0 * factor));
    buf.data[16] = u32(log2(511.0 * factor));
    buf.data[17] = u32(log2(512.0 * factor));
    buf.data[18] = u32(log2(4294967295.0 * factor));
})";

    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.computeStage.module = utils::CreateShaderModule(device, shader.c_str());
    csDesc.computeStage.entryPoint = "main";
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&csDesc);

    wgpu::BindGroup bindGroup =
        utils::MakeBindGroup(device, pipeline.GetBindGroupLayout(0), {{0, buffer}});

    wgpu::CommandBuffer commands;
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
        pass.SetPipeline(pipeline);
        pass.SetBindGroup(0, bindGroup);
        pass.Dispatch(1);
        pass.EndPass();

        commands = encoder.Finish();
    }

    queue.Submit(1, &commands);

    EXPECT_BUFFER_U32_RANGE_EQ(expected.data(), buffer, 0, kSteps);
}

TEST_P(ShaderTests, BadWGSL) {
    DAWN_SKIP_TEST_IF(HasToggleEnabled("skip_validation"));

    std::string shader = R"(
I am an invalid shader and should never pass validation!
})";
    ASSERT_DEVICE_ERROR(utils::CreateShaderModule(device, shader.c_str()));
}

// Tests that shaders using non-struct function parameters and return values for shader stage I/O
// can compile and link successfully.
TEST_P(ShaderTests, WGSLParamIO) {
    std::string vertexShader = R"(
[[stage(vertex)]]
fn main([[builtin(vertex_index)]] VertexIndex : u32) -> [[builtin(position)]] vec4<f32> {
    let pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
        vec2<f32>(-1.0,  1.0),
        vec2<f32>( 1.0,  1.0),
        vec2<f32>( 0.0, -1.0));
    return vec4<f32>(pos[VertexIndex], 0.0, 1.0);
})";
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, vertexShader.c_str());

    std::string fragmentShader = R"(
[[stage(fragment)]]
fn main([[builtin(position)]] fragCoord : vec4<f32>) -> [[location(0)]] vec4<f32> {
    return vec4<f32>(fragCoord.xy, 0.0, 1.0);
})";
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, fragmentShader.c_str());

    utils::ComboRenderPipelineDescriptor2 rpDesc;
    rpDesc.vertex.module = vsModule;
    rpDesc.cFragment.module = fsModule;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline2(&rpDesc);
}

// Tests that a vertex shader using struct function parameters and return values for shader stage
// I/O can compile and link successfully against a fragement shader using compatible non-struct I/O.
TEST_P(ShaderTests, WGSLMixedStructParamIO) {
    std::string vertexShader = R"(
struct VertexIn {
    [[location(0)]] position : vec3<f32>;
    [[location(1)]] color : vec4<f32>;
};

struct VertexOut {
    [[location(0)]] color : vec4<f32>;
    [[builtin(position)]] position : vec4<f32>;
};

[[stage(vertex)]]
fn main(input : VertexIn) -> VertexOut {
    var output : VertexOut;
    output.position = vec4<f32>(input.position, 1.0);
    output.color = input.color;
    return output;
})";
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, vertexShader.c_str());

    std::string fragmentShader = R"(
[[stage(fragment)]]
fn main([[location(0)]] color : vec4<f32>) -> [[location(0)]] vec4<f32> {
    return color;
})";
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, fragmentShader.c_str());

    utils::ComboRenderPipelineDescriptor2 rpDesc;
    rpDesc.vertex.module = vsModule;
    rpDesc.cFragment.module = fsModule;
    rpDesc.vertex.bufferCount = 1;
    rpDesc.cBuffers[0].attributeCount = 2;
    rpDesc.cBuffers[0].arrayStride = 28;
    rpDesc.cAttributes[0].shaderLocation = 0;
    rpDesc.cAttributes[0].format = wgpu::VertexFormat::Float32x3;
    rpDesc.cAttributes[1].shaderLocation = 1;
    rpDesc.cAttributes[1].format = wgpu::VertexFormat::Float32x4;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline2(&rpDesc);
}

// Tests that shaders using struct function parameters and return values for shader stage I/O
// can compile and link successfully.
TEST_P(ShaderTests, WGSLStructIO) {
    std::string vertexShader = R"(
struct VertexIn {
    [[location(0)]] position : vec3<f32>;
    [[location(1)]] color : vec4<f32>;
};

struct VertexOut {
    [[location(0)]] color : vec4<f32>;
    [[builtin(position)]] position : vec4<f32>;
};

[[stage(vertex)]]
fn main(input : VertexIn) -> VertexOut {
    var output : VertexOut;
    output.position = vec4<f32>(input.position, 1.0);
    output.color = input.color;
    return output;
})";
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, vertexShader.c_str());

    std::string fragmentShader = R"(
struct FragmentIn {
    [[location(0)]] color : vec4<f32>;
    [[builtin(position)]] fragCoord : vec4<f32>;
};

[[stage(fragment)]]
fn main(input : FragmentIn) -> [[location(0)]] vec4<f32> {
    return input.color * input.fragCoord;
})";
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, fragmentShader.c_str());

    utils::ComboRenderPipelineDescriptor2 rpDesc;
    rpDesc.vertex.module = vsModule;
    rpDesc.cFragment.module = fsModule;
    rpDesc.vertex.bufferCount = 1;
    rpDesc.cBuffers[0].attributeCount = 2;
    rpDesc.cBuffers[0].arrayStride = 28;
    rpDesc.cAttributes[0].shaderLocation = 0;
    rpDesc.cAttributes[0].format = wgpu::VertexFormat::Float32x3;
    rpDesc.cAttributes[1].shaderLocation = 1;
    rpDesc.cAttributes[1].format = wgpu::VertexFormat::Float32x4;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline2(&rpDesc);
}

// Tests that shaders I/O structs that us compatible locations but are not sorted by hand can link.
TEST_P(ShaderTests, WGSLUnsortedStructIO) {
    std::string vertexShader = R"(
struct VertexIn {
    [[location(0)]] position : vec3<f32>;
    [[location(1)]] color : vec4<f32>;
};

struct VertexOut {
    [[builtin(position)]] position : vec4<f32>;
    [[location(0)]] color : vec4<f32>;
};

[[stage(vertex)]]
fn main(input : VertexIn) -> VertexOut {
    var output : VertexOut;
    output.position = vec4<f32>(input.position, 1.0);
    output.color = input.color;
    return output;
})";
    wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, vertexShader.c_str());

    std::string fragmentShader = R"(
struct FragmentIn {
    [[location(0)]] color : vec4<f32>;
    [[builtin(position)]] fragCoord : vec4<f32>;
};

[[stage(fragment)]]
fn main(input : FragmentIn) -> [[location(0)]] vec4<f32> {
    return input.color * input.fragCoord;
})";
    wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, fragmentShader.c_str());

    utils::ComboRenderPipelineDescriptor2 rpDesc;
    rpDesc.vertex.module = vsModule;
    rpDesc.cFragment.module = fsModule;
    rpDesc.vertex.bufferCount = 1;
    rpDesc.cBuffers[0].attributeCount = 2;
    rpDesc.cBuffers[0].arrayStride = 28;
    rpDesc.cAttributes[0].shaderLocation = 0;
    rpDesc.cAttributes[0].format = wgpu::VertexFormat::Float32x3;
    rpDesc.cAttributes[1].shaderLocation = 1;
    rpDesc.cAttributes[1].format = wgpu::VertexFormat::Float32x4;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline2(&rpDesc);
}

// Tests that shaders I/O structs can be shared between vertex and fragment shaders.
TEST_P(ShaderTests, WGSLSharedStructIO) {
    // TODO(tint:714): Not yet implemeneted in tint yet, but intended to work.
    DAWN_SKIP_TEST_IF(IsD3D12() || IsVulkan() || IsMetal() || IsOpenGL() || IsOpenGLES());

    std::string shader = R"(
struct VertexIn {
    [[location(0)]] position : vec3<f32>;
    [[location(1)]] color : vec4<f32>;
};

struct VertexOut {
    [[location(0)]] color : vec4<f32>;
    [[builtin(position)]] position : vec4<f32>;
};

[[stage(vertex)]]
fn vertexMain(input : VertexIn) -> VertexOut {
    var output : VertexOut;
    output.position = vec4<f32>(input.position, 1.0);
    output.color = input.color;
    return output;
}

[[stage(fragment)]]
fn fragmentMain(input : VertexOut) -> [[location(0)]] vec4<f32> {
    return input.color;
})";
    wgpu::ShaderModule shaderModule = utils::CreateShaderModule(device, shader.c_str());

    utils::ComboRenderPipelineDescriptor2 rpDesc;
    rpDesc.vertex.module = shaderModule;
    rpDesc.vertex.entryPoint = "vertexMain";
    rpDesc.cFragment.module = shaderModule;
    rpDesc.cFragment.entryPoint = "fragmentMain";
    rpDesc.vertex.bufferCount = 1;
    rpDesc.cBuffers[0].attributeCount = 2;
    rpDesc.cBuffers[0].arrayStride = 28;
    rpDesc.cAttributes[0].shaderLocation = 0;
    rpDesc.cAttributes[0].format = wgpu::VertexFormat::Float32x3;
    rpDesc.cAttributes[1].shaderLocation = 1;
    rpDesc.cAttributes[1].format = wgpu::VertexFormat::Float32x4;
    wgpu::RenderPipeline pipeline = device.CreateRenderPipeline2(&rpDesc);
}

DAWN_INSTANTIATE_TEST(ShaderTests,
                      D3D12Backend(),
                      MetalBackend(),
                      OpenGLBackend(),
                      OpenGLESBackend(),
                      VulkanBackend());
