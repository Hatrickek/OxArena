#include "RuntimeLayer.h"

#include <imgui.h>
#include <Assets/AssetManager.h>
#include <Render/Vulkan/VulkanRenderer.h>
#include <Scene/SceneSerializer.h>
#include <Utils/ImGuiScoped.h>
#include <entt.hpp>

#include "Systems/CharacterSystem.h"
#include "Systems/FPSCamera.h"
#include "Systems/FreeCamera.h"

namespace OxylusRuntime {
  using namespace Oxylus;
  RuntimeLayer* RuntimeLayer::s_Instance = nullptr;

  RuntimeLayer::RuntimeLayer() : Layer("Game Layer") {
    s_Instance = this;
  }

  RuntimeLayer::~RuntimeLayer() = default;

  void RuntimeLayer::OnAttach(EventDispatcher& dispatcher) {
    auto& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_Left;

    dispatcher.sink<ReloadSceneEvent>().connect<&RuntimeLayer::OnSceneReload>(*this);
    LoadScene();
  }

  void RuntimeLayer::OnDetach() { }

  void RuntimeLayer::OnUpdate(Timestep deltaTime) {
    m_Scene->OnRuntimeUpdate(deltaTime);
  }

  void RuntimeLayer::OnImGuiRender() {
    RenderFinalImage();
    m_Scene->OnImGuiRender(Application::GetTimestep());

    constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                              ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    constexpr float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 window_pos;
    window_pos.x = (work_pos.x + PAD);
    window_pos.y = work_pos.y + PAD;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("Performance Overlay", nullptr, window_flags)) {
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / (float)ImGui::GetIO().Framerate, (float)ImGui::GetIO().Framerate);
      ImGui::End();
    }
  }

  void RuntimeLayer::LoadScene() {
    m_Scene = CreateRef<Scene>();
    const SceneSerializer serializer(m_Scene);
    serializer.Deserialize(GetAssetsPath("Scenes/Main.oxscene"));

    m_Scene->OnRuntimeStart();

    m_Scene->AddSystem<CharacterSystem>();
  }

  bool RuntimeLayer::OnSceneReload(ReloadSceneEvent&) {
    LoadScene();
    OX_CORE_INFO("Scene reloaded.");
    return true;
  }

  /*TODO(hatrickek): Proper way to render the final image internally as fullscreen without needing this.
  This a "hack" to render the final image as a fullscreen image.
  Currently the final image in engine renderer is rendered to an offscreen framebuffer image.*/
  void RuntimeLayer::RenderFinalImage() const {
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                                       | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiScoped::StyleVar style(ImGuiStyleVar_WindowPadding, ImVec2{});
    if (ImGui::Begin("FinalImage", nullptr, flags)) {
      ImGui::Image(m_Scene->GetRenderer().GetRenderPipeline()->GetFinalImage().GetDescriptorSet(),
        ImVec2{(float)Window::GetWidth(), (float)Window::GetHeight()});
      ImGui::End();
    }
  }
}
