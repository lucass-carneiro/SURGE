use super::VulkanContext;
use crate::errors::VulkanError;
use ash::vk;
use std::ffi::CStr;
use std::ptr::null;

#[derive(Default)]
pub struct GraphicsPipelineBuilder<'a> {
    shader_stage_infos: Vec<vk::PipelineShaderStageCreateInfo<'a>>,

    input_assembly_info: vk::PipelineInputAssemblyStateCreateInfo<'a>,
    rasterizer_info: vk::PipelineRasterizationStateCreateInfo<'a>,
    multisampling_info: vk::PipelineMultisampleStateCreateInfo<'a>,
    depth_info: vk::PipelineDepthStencilStateCreateInfo<'a>,
    render_info: vk::PipelineRenderingCreateInfo<'a>,

    blend_attachment: vk::PipelineColorBlendAttachmentState,
    color_attachment_format: vk::Format,
    pipeline_layout: vk::PipelineLayout,
}

impl<'a> GraphicsPipelineBuilder<'a> {
    pub fn set_layout(mut self, layout: vk::PipelineLayout) -> Self {
        self.pipeline_layout = layout;
        self
    }

    pub fn set_shaders(
        mut self,
        vertex_shader: vk::ShaderModule,
        fragment_sader: vk::ShaderModule,
        vertex_shader_name: &'a CStr,
        fragmen_shader_name: &'a CStr,
    ) -> Self {
        // Vertex Shader
        let vs_info = vk::PipelineShaderStageCreateInfo::default()
            .stage(vk::ShaderStageFlags::VERTEX)
            .module(vertex_shader)
            .name(vertex_shader_name);
        self.shader_stage_infos.push(vs_info);

        // Fragment Shader
        let fs_info = vk::PipelineShaderStageCreateInfo::default()
            .stage(vk::ShaderStageFlags::FRAGMENT)
            .module(fragment_sader)
            .name(fragmen_shader_name);
        self.shader_stage_infos.push(fs_info);
        self
    }

    pub fn set_input_topology(mut self, topology: vk::PrimitiveTopology) -> Self {
        self.input_assembly_info.topology = topology;
        self.input_assembly_info.primitive_restart_enable = vk::FALSE;
        self
    }

    pub fn set_polygon_mode(mut self, mode: vk::PolygonMode) -> Self {
        self.rasterizer_info.polygon_mode = mode;
        self.rasterizer_info.line_width = 1.0;
        self
    }

    pub fn set_cull_mode(
        mut self,
        cull_mode: vk::CullModeFlags,
        front_face: vk::FrontFace,
    ) -> Self {
        self.rasterizer_info.cull_mode = cull_mode;
        self.rasterizer_info.front_face = front_face;
        self
    }

    pub fn set_multisampling_none(mut self) -> Self {
        self.multisampling_info.sample_shading_enable = vk::FALSE;

        // Multisampling defaulted to no multisampling (1 sample per pixel)
        self.multisampling_info.rasterization_samples = vk::SampleCountFlags::TYPE_1;
        self.multisampling_info.min_sample_shading = 1.0;
        self.multisampling_info.p_sample_mask = null();

        // No alpha to coverage either
        self.multisampling_info.alpha_to_coverage_enable = vk::FALSE;
        self.multisampling_info.alpha_to_one_enable = vk::FALSE;
        self
    }

    pub fn set_blending_none(mut self) -> Self {
        // Default write mask
        self.blend_attachment.color_write_mask = vk::ColorComponentFlags::R
            | vk::ColorComponentFlags::G
            | vk::ColorComponentFlags::B
            | vk::ColorComponentFlags::A;

        // No blending
        self.blend_attachment.blend_enable = vk::FALSE;
        self
    }

    pub fn set_blending_additive(mut self) -> Self {
        self.blend_attachment.color_write_mask = vk::ColorComponentFlags::R
            | vk::ColorComponentFlags::G
            | vk::ColorComponentFlags::B
            | vk::ColorComponentFlags::A;

        self.blend_attachment.blend_enable = vk::TRUE;
        self.blend_attachment.src_color_blend_factor = vk::BlendFactor::SRC_ALPHA;
        self.blend_attachment.dst_color_blend_factor = vk::BlendFactor::ONE;
        self.blend_attachment.color_blend_op = vk::BlendOp::ADD;
        self.blend_attachment.src_alpha_blend_factor = vk::BlendFactor::ONE;
        self.blend_attachment.dst_alpha_blend_factor = vk::BlendFactor::ZERO;
        self.blend_attachment.alpha_blend_op = vk::BlendOp::ADD;
        self
    }

    pub fn set_blending_alpha(mut self) -> Self {
        self.blend_attachment.color_write_mask = vk::ColorComponentFlags::R
            | vk::ColorComponentFlags::G
            | vk::ColorComponentFlags::B
            | vk::ColorComponentFlags::A;

        self.blend_attachment.blend_enable = vk::TRUE;
        self.blend_attachment.src_color_blend_factor = vk::BlendFactor::SRC_ALPHA;
        self.blend_attachment.dst_color_blend_factor = vk::BlendFactor::ONE_MINUS_SRC_ALPHA;
        self.blend_attachment.color_blend_op = vk::BlendOp::ADD;
        self.blend_attachment.src_alpha_blend_factor = vk::BlendFactor::ONE;
        self.blend_attachment.dst_alpha_blend_factor = vk::BlendFactor::ZERO;
        self.blend_attachment.alpha_blend_op = vk::BlendOp::ADD;
        self
    }

    pub fn set_color_attachment_format(mut self, format: vk::Format) -> Self {
        self.color_attachment_format = format;
        self.render_info.color_attachment_count = 1;
        self.render_info.p_color_attachment_formats = &self.color_attachment_format;
        self
    }

    pub fn set_depth_format(mut self, format: vk::Format) -> Self {
        self.render_info.depth_attachment_format = format;
        self
    }

    pub fn set_depth_test_enabled(mut self, enable_write: bool, op: vk::CompareOp) -> Self {
        self.depth_info.depth_test_enable = vk::TRUE;
        self.depth_info.depth_write_enable = enable_write as u32;
        self.depth_info.depth_compare_op = op;
        self.depth_info.depth_bounds_test_enable = vk::TRUE;
        self.depth_info.stencil_test_enable = vk::FALSE;
        self.depth_info.front = vk::StencilOpState {
            ..Default::default()
        };
        self.depth_info.back = vk::StencilOpState {
            ..Default::default()
        };
        self.depth_info.min_depth_bounds = 0.0;
        self.depth_info.max_depth_bounds = 1.0;
        self
    }

    pub fn set_depth_test_disabled(mut self) -> Self {
        self.depth_info.depth_test_enable = vk::FALSE;
        self.depth_info.depth_write_enable = vk::FALSE;
        self.depth_info.depth_compare_op = vk::CompareOp::NEVER;
        self.depth_info.depth_bounds_test_enable = vk::FALSE;
        self.depth_info.stencil_test_enable = vk::FALSE;
        self.depth_info.front = vk::StencilOpState {
            ..Default::default()
        };
        self.depth_info.back = vk::StencilOpState {
            ..Default::default()
        };
        self.depth_info.min_depth_bounds = 0.0;
        self.depth_info.max_depth_bounds = 1.0;
        self
    }
}

impl VulkanContext {
    pub fn create_graphics_pipeline(
        &self,
        mut builder: GraphicsPipelineBuilder,
    ) -> Result<vk::Pipeline, VulkanError> {
        // Make viewport state from our stored viewport and scissor.
        // At the moment we wont support multiple viewports or scissors
        let viewport = vk::PipelineViewportStateCreateInfo {
            viewport_count: 1,
            scissor_count: 1,
            // TODO: viewport here
            ..Default::default()
        };

        // Color blending
        let blending = vk::PipelineColorBlendStateCreateInfo {
            logic_op_enable: vk::FALSE,
            logic_op: vk::LogicOp::COPY,
            attachment_count: 1,
            p_attachments: &builder.blend_attachment,
            ..Default::default()
        };

        // Completely clear VertexInputStateCreateInfo, as we have no need for it becase we use vertex pulling
        let vertex_input_info = vk::PipelineVertexInputStateCreateInfo {
            ..Default::default()
        };

        // Dynamic state info
        let state = [vk::DynamicState::VIEWPORT, vk::DynamicState::SCISSOR];

        let dynamic_info = vk::PipelineDynamicStateCreateInfo {
            dynamic_state_count: state.len() as u32,
            p_dynamic_states: state.as_ptr(),
            ..Default::default()
        };

        // Build the actual pipeline
        let pipeline_info = vk::GraphicsPipelineCreateInfo::default()
            .stages(builder.shader_stage_infos.as_slice())
            .vertex_input_state(&vertex_input_info)
            .input_assembly_state(&builder.input_assembly_info)
            .viewport_state(&viewport)
            .rasterization_state(&builder.rasterizer_info)
            .multisample_state(&builder.multisampling_info)
            .color_blend_state(&blending)
            .depth_stencil_state(&builder.depth_info)
            .layout(builder.pipeline_layout)
            .dynamic_state(&dynamic_info)
            .push_next(&mut builder.render_info);

        let pipelines = unsafe {
            self.device
                .create_graphics_pipelines(vk::PipelineCache::null(), &[pipeline_info], None)
                .map_err(|e| VulkanError::GraphicsPipelineCreationError(e.1))
        }?;

        Ok(pipelines[0])
    }

    pub fn destroy_pipeline(&self, pipeline: vk::Pipeline) {
        unsafe {
            self.device.destroy_pipeline(pipeline, None);
        }
    }
}
