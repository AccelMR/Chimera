#include "chBaseApplication.h"
#include "chDebug.h"
#include "chPath.h"

#include <format>
#include <execinfo.h>

using namespace chEngineSDK;

//#define CH_WINDOW_ONLY

#ifdef CH_WINDOW_ONLY
struct Vertex
{
  Vector4 position;
};

struct MatrixViewProj {
  Matrix4 View;
  Matrix4 Proj;
};

const uint32 defaultWidth = 1920;
const uint32 defaultHeight = 1080;
#endif //CH_WINDOW_ONLY


#define CH_BASE_APPLICATION

#ifdef CH_BASE_APPLICATION
int main(int argc, char** argv) {
  try
  {
    BaseApplication::startUp();
    auto& app = BaseApplication::instance();
    app.initialize(argc, argv);
    app.run();
    BaseApplication::shutDown();
  }
  catch(const std::exception& e)
  {
    LOG_ERROR(e.what());
    
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (int i = 0; i < frames; ++i) {
      LOG_FATAL(strs[i]);
    }
    free(strs);

    g_Debug().saveLog("resources/engine/logs/CRASHED_chCoreTestMain.txt");
    g_Debug().saveLogAsHtml("resources/engine/logs/CRASHED_chCoreTestMain.html");
    return 0;
  }

  g_Debug().saveLog("resources/engine/logs/chCoreTestMain.txt");
  return 1;
}
#endif //CH_BASE_APPLICATION

#ifdef CH_WINDOW_ONLY
int main(){
  ScreenModule::startUp();
  InputModule::startUp();
  SystemEventDipatcherModule::startUp();
  DynamicLibraryManager::startUp();

  auto [InputDispatcher, SystemDispatcher, DLLMngr] = std::tie(InputModule::instance(),
    SystemEventDipatcherModule::instance(),
    DynamicLibraryManager::instance());

  SCREEN_DESC winDesc;
  winDesc.name = "ChimeraCoreUnitTest";
  winDesc.title = "Chimera Core Unit Test";
  winDesc.width = defaultWidth;
  winDesc.height = defaultHeight;

  auto Eventhandler = ch_shared_ptr_new<ScreenEventHandle>();
  auto screen = ScreenModule::instance().createScreen(winDesc, Eventhandler);
  bool running = true;

  auto perspectiveMatrix = PerspectiveMatrix(Radian(0.7853982f), defaultWidth, defaultHeight, 0.1f, 100.0f);
  Vector3 camPos(0.0f, -15.0f, 5.0f);
  auto viewMatrix = LookAtMatrix(camPos, Vector3::ZERO, Vector3::UP);
  MatrixViewProj MVP{
    .View = viewMatrix.getTransposed(),
    .Proj = perspectiveMatrix.getTransposed()
  };

  auto MoveCamera = [&](const Vector3 dir) -> void {
    camPos += dir * 0.01667f;
    auto newPos = TranslationMatrix(camPos);
    MVP.View *= newPos;

    //LOG_INFO(StringUtils::format("{0}", newPos, newPos, newPos));
  };

  auto OnW = InputDispatcher.listenWKeyDown([&](){LOG_INFO("W Down."); MoveCamera(Vector3::FORWARD);});
  auto OnA = InputDispatcher.listenAKeyDown([&](){LOG_INFO("A Up.");MoveCamera(Vector3::LEFT);});
  auto OnS = InputDispatcher.listenSKeyDown([&](){LOG_INFO("S pressed.");MoveCamera(Vector3::BACKWARD);});
  auto OnD = InputDispatcher.listenDKeyDown([&](){LOG_INFO("D pressed.");MoveCamera(Vector3::RIGHT);});

  auto OnClose = SystemDispatcher.listenOnClose([&]() {
    screen->close();
    running = false;
  });

  auto OnResize = SystemDispatcher.listenOnResize([&](uint32 _width, uint32 _height) {
    String msg = StringUtils::format("Widht:{0}\nHeight:{1}", _width, _height);
    LOG_INFO(msg);
  });

  /************************************************************************/
  /*
   * Graphics init
   */                                                                     
  /************************************************************************/
  while (running) {
    Eventhandler->update();
    SystemDispatcher.dispatchEvents(Eventhandler);
    InputDispatcher.dispatchEvents(Eventhandler);
  }

  ScreenModule::shutDown();
  InputModule::shutDown();
  SystemEventDipatcherModule::shutDown();
  return 1;
}
#endif //CH_WINDOW_ONLY

#ifdef CH_GRAPHICS_USE
int main(int argc, char** argv) {
  ScreenModule::startUp();
  InputModule::startUp();
  SystemEventDipatcherModule::startUp();
  DynamicLibraryManager::startUp();
  CommandParser::getInstance().parse(static_cast<int32>(argc), argv);

  auto [InputDispatcher, SystemDispatcher, DLLMngr] = std::tie(InputModule::instance(),
    SystemEventDipatcherModule::instance(),
    DynamicLibraryManager::instance());

  SCREEN_DESC winDesc;
  winDesc.name = "ChimeraCoreUnitTest";
  winDesc.title = "Chimera Core Unit Test";
  winDesc.width = defaultWidth;
  winDesc.height = defaultHeight;

  auto Eventhandler = ch_shared_ptr_new<ScreenEventHandle>();
  auto screen = ScreenModule::instance().createScreen(winDesc, Eventhandler);
  bool running = true;

  auto perspectiveMatrix = PerspectiveMatrix(Radian(0.7853982f), defaultWidth, defaultHeight, 0.1f, 100.0f);
  Vector3 camPos(0.0f, -15.0f, 5.0f);
  auto viewMatrix = LookAtMatrix(camPos, Vector3::ZERO, Vector3::UP);
  MatrixViewProj MVP{
    .View = viewMatrix.getTransposed(),
    .Proj = perspectiveMatrix.getTransposed()
  };

  auto MoveCamera = [&](const Vector3 dir) -> void {
    camPos += dir * 0.01667f;
    auto newPos = TranslationMatrix(camPos);
    MVP.View *= newPos;

    //LOG_INFO(StringUtils::format("{0}", newPos, newPos, newPos));
  };

  auto OnW = InputDispatcher.listenWKeyDown([&](){LOG_INFO("W Down."); MoveCamera(Vector3::FORWARD);});
  auto OnA = InputDispatcher.listenAKeyDown([&](){LOG_INFO("A Up.");MoveCamera(Vector3::LEFT);});
  auto OnS = InputDispatcher.listenSKeyDown([&](){LOG_INFO("S pressed.");MoveCamera(Vector3::BACKWARD);});
  auto OnD = InputDispatcher.listenDKeyDown([&](){LOG_INFO("D pressed.");MoveCamera(Vector3::RIGHT);});

  auto OnClose = SystemDispatcher.listenOnClose([&]() {
    screen->close();
    running = false;
  });

  auto OnResize = SystemDispatcher.listenOnResize([&](uint32 _width, uint32 _height) {
    String msg(std::format("Widht:{0}\nHeight:{1}", _width, _height));
    LOG_INFO(msg);
  });

#if USING(CH_PLATFORM_LINUX)
  char exePath[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);

  if (count == -1) {
    std::cerr << "Error: No se pudo obtener la ruta del ejecutable." << std::endl;
    return 1;
  }
  const String exeDir = String(exePath).substr(0, String(exePath).find_last_of("/"));
  const String graphicsAPIName =  exeDir + "/" + std::format("ch{0}Graphics", CommandParser::getInstance().getParam("GraphicsAPI", "Vulkan"));
#else
  const String graphicsAPIName = std::format("ch{0}Graphics", CommandParser::getInstance().getParam("GraphicsAPI", "Vulkan"));
#endif //USING(CH_PLATFORM_LINUX)

  auto graphicsDll = DLLMngr.loadDynLibrary(graphicsAPIName).lock();
  CH_ASSERT(graphicsDll);

  auto loadPlugin = reinterpret_cast<void(*)(const SPtr<Screen>)>(graphicsDll->getSymbol("loadPlugin"));
  CH_ASSERT(loadPlugin);
  loadPlugin(screen);

  //These were started on the previous call loadPlugin()
  auto [GraphicAPI, GPUResourceMngr] = std::tie(GraphicsModule::instance(), GPUResourceModule::instance());

  LOG_INFO("Graphics API loaded in main program.");

  /************************************************************************/
  /*
   * Graphics init
   */                                                                     
  /************************************************************************/
  Vector<uint8> shaderFile = FileSystem::fastReadFile({"../../../../Resources/Shaders/Basic_VS_PS.hlsl"});
  auto swapchain = GraphicAPI.getSwapChain();

  auto VS_PS = GPUResourceMngr.createShader({
    .name = StringID("BasicShader"),
    .VS = {
      .byteCode = shaderFile,
      .entryFunc = "VSMain"
     },
    .PS = {
      .byteCode = shaderFile,
      .entryFunc = "PSMain"
     }
  });

  auto MVPBuffer = GPUResourceMngr.createBuffer(sizeof(MatrixViewProj));

  MVPBuffer->update(sizeof(MatrixViewProj), &MVP);

  auto pipeline = GraphicAPI.createPipelineState({
    .VS = VS_PS,
    .PS = VS_PS,
    .rasterizerStateDesc = {
      .cullMode = CULL_MODE::kNONE
    },
    .vertexBufferBindingsDesc = {
      {
        VertexBufferBindingDesc(VERTEX_SEMANTIC::kPOSITION, 0, 0, FORMAT::kR32G32B32A32_FLOAT)
      }
    },
    .bindingGroups {
      .buffers = {
        { BufferBindingDesc::TYPE::kUNIFORM, SHADER_STAGE::ALL, 0, sizeof(Matrix4), MVPBuffer }
      }
    }
  });
  auto commandBuffer = GraphicAPI.beginCommandRecording(COMMAND_BUFFER_TYPES::kDIRECT,
                                                        pipeline);

  GPUBarrier barrierStart;
  barrierStart.transition.stateBefore = chGPUDesc::ResourceStates::kPRESENT;
  barrierStart.transition.stateAfter = chGPUDesc::ResourceStates::kRENDER_TARGET;

  GPUBarrier barrierEnd = {};
  barrierEnd.transition.stateBefore = chGPUDesc::ResourceStates::kRENDER_TARGET;
  barrierEnd.transition.stateAfter = chGPUDesc::ResourceStates::kPRESENT;

  GraphicAPI.createFence();

  const AABox box(-Vector3::UNIT, Vector3::UNIT);
  auto boxVertices = box.generateVertices4();
  VertexBufferDesc vertexBuffDesc{
    .strideInBytes = sizeof(Vertex),
    .size = static_cast<uint32>(boxVertices.size() * sizeof(Vertex))
  };
  auto vertexBuffer = GPUResourceMngr.createVertexBuffer(vertexBuffDesc);
  vertexBuffer->update(vertexBuffDesc, &boxVertices[0]);

  auto boxIndices = box.getConstIndices();
  IndexBufferDesc indexBufferDesc = {
    .format = FORMAT::kR16_UINT,
    .size = static_cast<uint32>(boxIndices.size() * sizeof(uint32))
  };
  auto indexBuffer = GPUResourceMngr.createIndexBuffer(indexBufferDesc);
  indexBuffer->update(indexBufferDesc, &boxIndices[0]);

  const Box2D viewport({0.0f, 0.0f}, { defaultWidth, defaultHeight });

  while (running) {
    Eventhandler->update();
    SystemDispatcher.dispatchEvents(Eventhandler);
    InputDispatcher.dispatchEvents(Eventhandler);

    commandBuffer->reset(pipeline);
    commandBuffer->setPipeLineState(pipeline);

    commandBuffer->setSwapChain();

    commandBuffer->setScissorRect(viewport);
    commandBuffer->setRect(viewport);

    commandBuffer->resourceBarrierSwapChain(barrierStart);

    commandBuffer->setGPUBuffer(MVPBuffer, 0);

    commandBuffer->clearSwapChainTexture(LinearColor::Pink);
    commandBuffer->setTopology(PRIMITIVE_TOPOLOGY_TYPE::kPRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    commandBuffer->setVertexBuffer(0, 1, vertexBuffer);
    commandBuffer->setIndexBuffer(indexBuffer);
    commandBuffer->drawIndexed(boxIndices.size(), 1, 0, 0, 0);

    commandBuffer->resourceBarrierSwapChain(barrierEnd);

    commandBuffer->close();

    GraphicAPI.executeCommandBuffers({ commandBuffer });

    GraphicAPI.present(0, 0);

    GraphicAPI.moveToNextFrame();
    GraphicAPI.waitGPU();
  }

  ScreenModule::shutDown();
  InputModule::shutDown();
  SystemEventDipatcherModule::shutDown();
  return 1;
}
#endif //CH_GRAPHICS_USE