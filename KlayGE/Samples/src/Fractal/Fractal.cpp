#include <KlayGE/KlayGE.hpp>
#include <KlayGE/ThrowErr.hpp>
#include <KlayGE/Util.hpp>
#include <KlayGE/GraphicsBuffer.hpp>
#include <KlayGE/Math.hpp>
#include <KlayGE/Font.hpp>
#include <KlayGE/Renderable.hpp>
#include <KlayGE/RenderableHelper.hpp>
#include <KlayGE/RenderEngine.hpp>
#include <KlayGE/RenderEffect.hpp>
#include <KlayGE/FrameBuffer.hpp>
#include <KlayGE/RenderLayout.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/Context.hpp>
#include <KlayGE/ResLoader.hpp>
#include <KlayGE/Texture.hpp>
#include <KlayGE/RenderSettings.hpp>

#include <KlayGE/RenderFactory.hpp>
#include <KlayGE/InputFactory.hpp>

#include <vector>
#include <sstream>

#include "Fractal.hpp"

using namespace std;
using namespace KlayGE;

int const WIDTH = 800;
int const HEIGHT = 600;

namespace
{
	class RenderFractal : public RenderableHelper
	{
	public:
		RenderFractal()
			: RenderableHelper(L"Fractal")
		{
			RenderFactory& rf = Context::Instance().RenderFactoryInstance();

			technique_ = rf.LoadEffect("Fractal.kfx")->TechniqueByName("Mandelbrot");

			float4 const & offset = rf.RenderEngineInstance().TexelToPixelOffset() * 2;

			float3 xyzs[] =
			{
				float3(-1 + offset.x() / WIDTH, +1 + offset.y() / HEIGHT, 0.5f),
				float3(+1 + offset.x() / WIDTH, +1 + offset.y() / HEIGHT, 0.5f),
				float3(-1 + offset.x() / WIDTH, -1 + offset.y() / HEIGHT, 0.5f),
				float3(+1 + offset.x() / WIDTH, -1 + offset.y() / HEIGHT, 0.5f),
			};

			float2 texs[] =
			{
				float2(0, 0),
				float2(1, 0),
				float2(0, 1),
				float2(1, 1),
			};

			rl_ = rf.MakeRenderLayout();
			rl_->TopologyType(RenderLayout::TT_TriangleStrip);

			ElementInitData init_data;
			init_data.row_pitch = sizeof(xyzs);
			init_data.slice_pitch = 0;
			init_data.data.resize(init_data.row_pitch);
			memcpy(&init_data.data[0], xyzs, init_data.row_pitch);
			GraphicsBufferPtr pos_vb = rf.MakeVertexBuffer(BU_Static, EAH_GPU_Read, &init_data);

			init_data.row_pitch = sizeof(texs);
			init_data.slice_pitch = 0;
			init_data.data.resize(init_data.row_pitch);
			memcpy(&init_data.data[0], texs, init_data.row_pitch);
			GraphicsBufferPtr tex0_vb = rf.MakeVertexBuffer(BU_Static, EAH_GPU_Read, &init_data);

			rl_->BindVertexStream(pos_vb, boost::make_tuple(vertex_element(VEU_Position, 0, EF_BGR32F)));
			rl_->BindVertexStream(tex0_vb, boost::make_tuple(vertex_element(VEU_TextureCoord, 0, EF_GR32F)));

			box_ = MathLib::compute_bounding_box<float>(&xyzs[0], &xyzs[0] + sizeof(xyzs) / sizeof(xyzs[0]));
		}

		void SetTexture(TexturePtr texture)
		{
			*(technique_->Effect().ParameterByName("fractal_sampler")) = texture;
		}
	};

	class RenderPlane : public RenderableHelper
	{
	public:
		RenderPlane()
			: RenderableHelper(L"Plane")
		{
			RenderFactory& rf = Context::Instance().RenderFactoryInstance();

			technique_ = rf.LoadEffect("Fractal.kfx")->TechniqueByName("Show");

			float3 xyzs[] =
			{
				float3(-1, +1, 0.5f),
				float3(+1, +1, 0.5f),
				float3(-1, -1, 0.5f),
				float3(+1, -1, 0.5f),
			};

			float2 texs[] =
			{
				float2(0, 0),
				float2(1, 0),
				float2(0, 1),
				float2(1, 1),
			};

			rl_ = rf.MakeRenderLayout();
			rl_->TopologyType(RenderLayout::TT_TriangleStrip);

			ElementInitData init_data;
			init_data.row_pitch = sizeof(xyzs);
			init_data.slice_pitch = 0;
			init_data.data.resize(init_data.row_pitch);
			memcpy(&init_data.data[0], xyzs, init_data.row_pitch);
			GraphicsBufferPtr pos_vb = rf.MakeVertexBuffer(BU_Static, EAH_GPU_Read, &init_data);

			init_data.row_pitch = sizeof(texs);
			init_data.slice_pitch = 0;
			init_data.data.resize(init_data.row_pitch);
			memcpy(&init_data.data[0], texs, init_data.row_pitch);
			GraphicsBufferPtr tex0_vb = rf.MakeVertexBuffer(BU_Static, EAH_GPU_Read, &init_data);

			rl_->BindVertexStream(pos_vb, boost::make_tuple(vertex_element(VEU_Position, 0, EF_BGR32F)));
			rl_->BindVertexStream(tex0_vb, boost::make_tuple(vertex_element(VEU_TextureCoord, 0, EF_GR32F)));

			box_ = MathLib::compute_bounding_box<float>(&xyzs[0], &xyzs[0] + sizeof(xyzs) / sizeof(xyzs[0]));
		}

		void SetTexture(TexturePtr texture)
		{
			*(technique_->Effect().ParameterByName("fractal_sampler")) = texture;
		}
	};

	bool ConfirmDevice()
	{
		RenderFactory& rf = Context::Instance().RenderFactoryInstance();
		RenderEngine& re = rf.RenderEngineInstance();
		RenderDeviceCaps const & caps = re.DeviceCaps();
		if (caps.max_shader_model < 2)
		{
			return false;
		}

		try
		{
			TexturePtr temp_tex = rf.MakeTexture2D(800, 600, 1, EF_GR16F, EAH_GPU_Read | EAH_GPU_Write, NULL);
			rf.Make2DRenderView(*temp_tex, 0);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}
}

int main()
{
	ResLoader::Instance().AddPath("../../media/Common");
	ResLoader::Instance().AddPath("../../media/Fractal");

	RenderSettings settings;
	SceneManagerPtr sm = Context::Instance().LoadCfg(settings, "KlayGE.cfg");
	settings.ConfirmDevice = ConfirmDevice;

	Fractal app("Fractal", settings);
	app.Create();
	app.Run();

	return 0;
}

Fractal::Fractal(std::string const & name, RenderSettings const & settings)
			: App3DFramework(name, settings)
{
}

void Fractal::InitObjects()
{
	font_ = Context::Instance().RenderFactoryInstance().MakeFont("gkai00mp.kfont", 16);

	RenderEngine& renderEngine(Context::Instance().RenderFactoryInstance().RenderEngineInstance());

	render_buffer_[0] = Context::Instance().RenderFactoryInstance().MakeFrameBuffer();
	render_buffer_[1] = Context::Instance().RenderFactoryInstance().MakeFrameBuffer();
	render_buffer_[0]->GetViewport().camera
		= render_buffer_[1]->GetViewport().camera = renderEngine.CurFrameBuffer()->GetViewport().camera;

	renderFractal_.reset(new RenderFractal);
	renderPlane_.reset(new RenderPlane);
}

void Fractal::OnResize(uint32_t width, uint32_t height)
{
	App3DFramework::OnResize(width, height);

	RenderFactory& rf = Context::Instance().RenderFactoryInstance();

	ElementInitData init_data;
	init_data.row_pitch = width * NumFormatBytes(EF_GR16F);
	init_data.data.assign(init_data.row_pitch * height, 0);
	init_data.slice_pitch = 0;

	try
	{
		rendered_tex_[0] = rf.MakeTexture2D(width, height, 1, EF_GR16F, EAH_GPU_Read | EAH_GPU_Write, &init_data);
	}
	catch (...)
	{
		init_data.row_pitch = width * NumFormatBytes(EF_ABGR16F);
		init_data.data.resize(init_data.row_pitch * height, 0);

		rendered_tex_[0] = rf.MakeTexture2D(width, height, 1, EF_ABGR16F, EAH_GPU_Read | EAH_GPU_Write, &init_data);
	}
	rendered_tex_[1] = rf.MakeTexture2D(width, height, 1, rendered_tex_[0]->Format(), EAH_GPU_Read | EAH_GPU_Write, &init_data);

	for (int i = 0; i < 2; ++ i)
	{
		render_buffer_[i]->Attach(FrameBuffer::ATT_Color0, rf.Make2DRenderView(*rendered_tex_[i], 0));
	}
}

uint32_t Fractal::DoUpdate(uint32_t pass)
{
	RenderEngine& renderEngine(Context::Instance().RenderFactoryInstance().RenderEngineInstance());

	static bool odd = false;

	switch (pass)
	{
	case 0:
		renderEngine.BindFrameBuffer(render_buffer_[!odd]);
		renderEngine.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color, Color(0.2f, 0.4f, 0.6f, 1), 1.0f, 0);

		checked_pointer_cast<RenderFractal>(renderFractal_)->SetTexture(rendered_tex_[odd]);
		renderFractal_->AddToRenderQueue();
		return App3DFramework::URV_Need_Flush;

	default:
		renderEngine.BindFrameBuffer(FrameBufferPtr());
		renderEngine.CurFrameBuffer()->Clear(FrameBuffer::CBM_Color | FrameBuffer::CBM_Depth, Color(0.2f, 0.4f, 0.6f, 1), 1.0f, 0);

		checked_pointer_cast<RenderPlane>(renderPlane_)->SetTexture(rendered_tex_[!odd]);
		renderPlane_->AddToRenderQueue();

		odd = !odd;

		std::wostringstream stream;
		stream << this->FPS();

		font_->RenderText(0, 0, Color(1, 1, 0, 1), L"GPU Fractal");
		font_->RenderText(0, 18, Color(1, 1, 0, 1), stream.str());
		return App3DFramework::URV_Need_Flush | App3DFramework::URV_Finished;
	}
}
