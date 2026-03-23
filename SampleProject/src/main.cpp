#include "Project.hpp"
#include "stb_image.hpp"
#include <concepts>

class Application {
private:
	struct UpdateFunc {
		Application* ptr;
		inline void operator()() {
			ptr->update();
		}
	};
	struct RenderFunc {
		Application* ptr;
		inline void operator()() {
			ptr->render();
		}
	};
	tx::RE::Framework<tx::RE::Mode::Release, UpdateFunc, RenderFunc> framework;

	bool initGLFW() {
		std::cout << "Initializing GLFW...\n";
		if (!glfwInit()) {
			std::cerr << "[FatalError]: Failed to init GLFW\n";
			return false;
		}
		return true;
	}
	bool initGLAD() {
		std::cout << "Initializing GLAD...\n";
		if (!gl::init((void*)glfwGetProcAddress)) {
			std::cerr << "[FatalError]: Failed to init GLAD\n";
			return false;
		}
		return true;
	}

	bool m_valid = 0;

public:
	Application() {
		if (!initGLFW()) return;
		framework = decltype(framework){
			UpdateFunc{ this },
			RenderFunc{ this }
		};
		if (!framework.valid()) return;
		if (!initGLAD()) return;
		if (!init()) return;
		m_valid = 1;
	}
	~Application() {
		glfwTerminate();
	}

	void run() { this->framework.run(); }
	bool valid() const { return m_valid; }

private:
	void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			switch (key) {
			}
		}
	}

	constexpr inline static thread_local re::SMStyle smtype = re::SMStyle::Pipeline;

	re::DrawCallManager dcm;
	re::ShaderManager<smtype> sm;
	re::VertexAttributeManager vam;
	re::BufferHandle<re::StaticBufferObject<tx::vec2>> squarePosBuffer;
	re::BufferHandle<re::StaticBufferObject<tx::vec2>> instanceBuffer;
	re::BufferHandle<re::StaticBufferObject<tx::vec2>> UVBuffer;
	re::BufferHandle<re::RingBufferObject<tx::u32>> animStateBuffer;
	re::BufferHandle<re::RingBufferObject<float>> scaleBuffer;
	re::TextureArray ta;


private:
	bool init() {
		tx::glfwSetKeyCallback<Application, &Application::onKeyEvent>(framework.getWindow(), this);
		tx::glEnableTransparent();

		vector<tx::vec2> squarePos = {
			{ 0.5, -0.5 },
			{ -0.5, -0.5 },
			{ 0.5, 0.5 },
			{ -0.5, -0.5 },
			{ 0.5, 0.5 },
			{ -0.5, 0.5 }
		};
		// bottom left origin
		// vector<tx::vec2> squarePos2 = {
		// 	{ 1.0, 0.0 }, // Bottom-Right
		// 	{ 0.0, 0.0 }, // Bottom-Left
		// 	{ 1.0, 1.0 }, // Top-Right
		// 	{ 0.0, 0.0 }, // Bottom-Left
		// 	{ 1.0, 1.0 }, // Top-Right
		// 	{ 0.0, 1.0 } // Top-Left
		// };

		vector<tx::vec2> positions = {
			tx::Origin
		};

		squarePosBuffer.bo.alloc(6, squarePos.data());
		instanceBuffer.bo.alloc(1, positions.data());
		UVBuffer.bo.alloc(6, squareUV.data());

		animStateBuffer.bo.alloc();
		scaleBuffer.bo.alloc();
		vam = tx::RE::VertexAttributeManager([&](tx::RE::VAMIniter& initer) {
			squarePosBuffer.id = initer.addAttrib<tx::vec2>();
			UVBuffer.id = initer.addAttrib<tx::vec2>();
			animStateBuffer.id = initer.addAttribInstanced<tx::u32>();
			instanceBuffer.id = initer.addAttribInstanced<tx::vec2>();
			scaleBuffer.id = initer.addAttribInstanced<tx::vec2>();
		});

		re::VAMBindBuffer(vam, squarePosBuffer);
		re::VAMBindBuffer(vam, UVBuffer);
		re::VAMBindBuffer(vam, animStateBuffer);
		re::VAMBindBuffer(vam, instanceBuffer);
		re::VAMBindBuffer(vam, scaleBuffer);


		re::ProgramId activeShaders;
		if (!re::addShaderPair("vertex", "fragment", sm, activeShaders)) {
			std::cerr << "[Error]: failed to add shaders.\n";
			return 0;
		}

		stbi_set_flip_vertically_on_load(true);
		int width, height, channels;
		std::vector<tx::u8*> data;
		data.reserve(11);

		for (int i = 1; i <= 12; i++) {
			if (i == 4) continue; // watch who ever is reading this code be so confusing...
			std::ostringstream oss;
			oss << "/home/TX_Jerry/Desktop/mpv-shot00" << std::setw(2) << std::setfill('0') << i << ".jpg";
			std::string path = oss.str();
			data.push_back(stbi_load(path.c_str(), &width, &height, &channels, 4));
			if (!data.back()) {
				std::cerr << "[Error]: stb_image failed to load image in frame: " << i << endl;
				return 0;
			}
		}
		tx::u32 length = width * height * 4;

		ta = re::TextureArray(re::TextureFormat::RGBA, { width, height }, data.size(), 0);
		ta.setTextureRule(re::TextureRule::Linear, re::TextureRule::Repeat);
		for (int i = 0; i < data.size(); i++) {
			ta.setLayer(i, std::bitSpan(data[i], length));
			if (data[i]) stbi_image_free(data[i]);
		}

		dcm = tx::RE::DrawCallManager();
		dcm.setVAM(vam);
		dcm.setShaders(sm.get(activeShaders));
		dcm.setTexture(0, ta);

		return 1;
	}

	tx::u32 frameCounter = 0;
	tx::u32 imageCount = 15;

	vector<tx::vec2> squareUV = {
		{ 1.0, 0.0 },
		{ 0.0, 0.0 },
		{ 1.0, 1.0 },
		{ 0.0, 0.0 },
		{ 1.0, 1.0 },
		{ 0.0, 1.0 }
	};

	const float scaleIncreaseMult = 1.067f;
	const float scaleDecreaseMult = 0.8f;
	float scale = 0.1f, currentMult = scaleIncreaseMult;

	tx::u64 tickCounter = 0;
	void update() {
		if (!(tickCounter % 3)) {
			re::writeRingBuffer(animStateBuffer.bo, [&](std::vector<tx::u32>& input) {
				for (int i = 0; i < 4; i++)
					input.push_back(frameCounter);
			});

			frameCounter++;
			if (frameCounter >= 11) {
				frameCounter = 0;
			}
		}
		if (tickCounter >= 10) {
			imageCount--;
			if (imageCount == 0) {
				imageCount = 15;
			}
			tickCounter = 0;
		}
		scale *= currentMult;
		re::writeRingBuffer(scaleBuffer.bo, [&](std::vector<float>& input) {
			input.push_back(scale);
		});
		if (scale >= 3.0f) {
			currentMult = scaleDecreaseMult;
		} else if (scale <= 0.5f) {
			currentMult = scaleIncreaseMult;
		}
		tickCounter++;
	}
	void render() {
		//tx::Time::Timer timer;
		re::VAMUpdateRingBuffer(vam, animStateBuffer);
		re::VAMUpdateRingBuffer(vam, scaleBuffer);
		dcm.drawInstanced(0, 6, 1);
		//cout << timer.duration() << "ms" << endl;
	}
};

// Things to add:
// 1. reconstruction of all "baked" class -> can "interit" the data from previous instance
// 2. DrawCallMan: error if not all dynamic buffer have same range

int main() {
	std::cout << "Initializing Application...\n";
	Application app;
	if (!app.valid()) {
		std::cerr << "[FatalError]: Failed to init Application\n";
		return 1;
	}

	std::cout << "[Status]: Main Loop Starts\n";
	app.run();

	std::cout << "[Status]: Terminating...\n";
	return 0;
}