#pragma once

#include <d3d11.h>

#include <cstring>

namespace mono_base::ui::d3d11 {

struct StateBackup {
    UINT scissor_rects_count = 0;
    UINT viewports_count = 0;
    D3D11_RECT scissor_rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};

    ID3D11RenderTargetView* rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    ID3D11DepthStencilView* dsv = nullptr;

    ID3D11BlendState* blend_state = nullptr;
    FLOAT blend_factor[4]{};
    UINT sample_mask = 0;

    ID3D11DepthStencilState* depth_state = nullptr;
    UINT stencil_ref = 0;

    ID3D11RasterizerState* raster_state = nullptr;

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11HullShader* hs = nullptr;
    ID3D11DomainShader* ds = nullptr;
    ID3D11ComputeShader* cs = nullptr;

    ID3D11ClassInstance* vs_instances[256]{};
    UINT vs_instances_count = 256;
    ID3D11ClassInstance* ps_instances[256]{};
    UINT ps_instances_count = 256;
    ID3D11ClassInstance* gs_instances[256]{};
    UINT gs_instances_count = 256;

    ID3D11Buffer* vs_constant_buffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
    ID3D11Buffer* ps_constant_buffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT]{};
    ID3D11SamplerState* ps_samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT]{};
    ID3D11ShaderResourceView* ps_srv[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};

    ID3D11InputLayout* input_layout = nullptr;
    ID3D11Buffer* index_buffer = nullptr;
    ID3D11Buffer* vertex_buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]{};
    UINT vertex_buffer_strides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]{};
    UINT vertex_buffer_offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT]{};
    DXGI_FORMAT index_format = DXGI_FORMAT_UNKNOWN;
    UINT index_offset = 0;
    D3D_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

    void backup(ID3D11DeviceContext* ctx) {
        scissor_rects_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        viewports_count = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx->RSGetScissorRects(&scissor_rects_count, scissor_rects);
        ctx->RSGetViewports(&viewports_count, viewports);

        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, &dsv);
        ctx->OMGetBlendState(&blend_state, blend_factor, &sample_mask);
        ctx->OMGetDepthStencilState(&depth_state, &stencil_ref);
        ctx->RSGetState(&raster_state);

        ctx->VSGetShader(&vs, vs_instances, &vs_instances_count);
        ctx->PSGetShader(&ps, ps_instances, &ps_instances_count);
        ctx->GSGetShader(&gs, gs_instances, &gs_instances_count);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        ctx->CSGetShader(&cs, nullptr, nullptr);

        ctx->VSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vs_constant_buffers);
        ctx->PSGetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ps_constant_buffers);
        ctx->PSGetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, ps_samplers);
        ctx->PSGetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, ps_srv);

        ctx->IAGetInputLayout(&input_layout);
        ctx->IAGetIndexBuffer(&index_buffer, &index_format, &index_offset);
        ctx->IAGetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
            vertex_buffers, vertex_buffer_strides, vertex_buffer_offsets);
        ctx->IAGetPrimitiveTopology(&topology);
    }

    void restore(ID3D11DeviceContext* ctx) {
        ctx->RSSetScissorRects(scissor_rects_count, scissor_rects);
        ctx->RSSetViewports(viewports_count, viewports);

        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, dsv);
        ctx->OMSetBlendState(blend_state, blend_factor, sample_mask);
        ctx->OMSetDepthStencilState(depth_state, stencil_ref);
        ctx->RSSetState(raster_state);

        ctx->VSSetShader(vs, vs_instances, vs_instances_count);
        ctx->PSSetShader(ps, ps_instances, ps_instances_count);
        ctx->GSSetShader(gs, gs_instances, gs_instances_count);
        ctx->HSSetShader(hs, nullptr, 0);
        ctx->DSSetShader(ds, nullptr, 0);
        ctx->CSSetShader(cs, nullptr, 0);

        ctx->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, vs_constant_buffers);
        ctx->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ps_constant_buffers);
        ctx->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, ps_samplers);
        ctx->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, ps_srv);

        ctx->IASetInputLayout(input_layout);
        ctx->IASetIndexBuffer(index_buffer, index_format, index_offset);
        ctx->IASetVertexBuffers(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT,
            vertex_buffers, vertex_buffer_strides, vertex_buffer_offsets);
        ctx->IASetPrimitiveTopology(topology);

        release();
    }

    void release() {
        if (blend_state) { blend_state->Release(); blend_state = nullptr; }
        if (depth_state) { depth_state->Release(); depth_state = nullptr; }
        if (raster_state) { raster_state->Release(); raster_state = nullptr; }
        if (vs) { vs->Release(); vs = nullptr; }
        if (ps) { ps->Release(); ps = nullptr; }
        if (gs) { gs->Release(); gs = nullptr; }
        if (hs) { hs->Release(); hs = nullptr; }
        if (ds) { ds->Release(); ds = nullptr; }
        if (cs) { cs->Release(); cs = nullptr; }
        if (input_layout) { input_layout->Release(); input_layout = nullptr; }
        if (index_buffer) { index_buffer->Release(); index_buffer = nullptr; }

        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            if (rtv[i]) { rtv[i]->Release(); rtv[i] = nullptr; }
        }
        if (dsv) { dsv->Release(); dsv = nullptr; }

        for (auto* buf : vs_constant_buffers) {
            if (buf) { buf->Release(); }
        }
        std::memset(vs_constant_buffers, 0, sizeof(vs_constant_buffers));

        for (auto* buf : ps_constant_buffers) {
            if (buf) { buf->Release(); }
        }
        std::memset(ps_constant_buffers, 0, sizeof(ps_constant_buffers));

        for (auto* samp : ps_samplers) {
            if (samp) { samp->Release(); }
        }
        std::memset(ps_samplers, 0, sizeof(ps_samplers));

        for (auto* srv : ps_srv) {
            if (srv) { srv->Release(); }
        }
        std::memset(ps_srv, 0, sizeof(ps_srv));

        for (auto* vb : vertex_buffers) {
            if (vb) { vb->Release(); }
        }
        std::memset(vertex_buffers, 0, sizeof(vertex_buffers));
    }
};

} // namespace mono_base::ui::d3d11
