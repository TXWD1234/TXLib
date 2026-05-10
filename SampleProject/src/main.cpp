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
	tx::RE::Framework<re::Mode::FixTickRate | re::Mode::PrintFrameRate, UpdateFunc, RenderFunc> framework;

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

	re::RE re;
	re::RSP rr;

private:
	bool init() {
		tx::glfwSetKeyCallback<Application, &Application::onKeyEvent>(framework.getWindow(), this);
		tx::glBasicSettings();
		stbi_set_flip_vertically_on_load(true);
		glfwSwapInterval(0); // turn off vsync

		re.init();
		rr = re.createSectionProxy(re::readShaderSource("vertex.vert"), re::readShaderSource("fragment.frag"));


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


		return 1;
	}

	tx::u32 frameCounter = 0;
	tx::u32 imageCount = 15;


	const float scaleIncreaseMult = 1.067f;
	const float scaleDecreaseMult = 0.9f;
	float scale = 0.1f, currentMult = scaleIncreaseMult;
	float degree = 0.0f, rotationSpeed = tx::ONE_DEGREE * -5.0f, degreeMax = 2 * tx::PI;
	tx::Rainbow colorEngine = tx::Rainbow(36);

	tx::u64 tickCounter = 0;
	void update() {
		// anim frame
		if (!(tickCounter % 3)) {
			frameCounter++;
			if (frameCounter >= 11) {
				frameCounter = 0;
			}
		}
		// scale
		scale *= currentMult;
		if (scale >= 2.0f) {
			currentMult = scaleDecreaseMult;
		} else if (scale <= 0.5f) {
			currentMult = scaleIncreaseMult;
		}
		// rotation
		degree += rotationSpeed;
		if (degree >= degreeMax) degree -= degreeMax;

		tickCounter++;
	}
	void render() {
		//rr.drawSprite(tx::Origin, anim.next(), tx::vec2{ 1.0f, 1.0f }, 0, colorEngine.getNextColor().compress());
		re.draw();
	}
};

int main() {

	// --- Test 1: Basic flat object ---
	{
		tx::JsonObject obj;
		obj.insert("name", tx::JsonValue(std::string("Alice")));
		obj.insert("age", tx::JsonValue(30));
		obj.insert("score", tx::JsonValue(9.5f));
		obj.insert("active", tx::JsonValue(true));

		std::string out;
		obj.write(out);
		std::cout << "[Test 1 - Flat object]\n"
		          << out << "\n\n";
		// Expected: {"name":"Alice","age":30,"score":9.5,"active":true}
	}

	// --- Test 2: Nested object ---
	{
		tx::JsonObject address;
		address.insert("city", tx::JsonValue(std::string("Toronto")));
		address.insert("zip", tx::JsonValue(std::string("M5V")));

		tx::JsonObject person;
		person.insert("name", tx::JsonValue(std::string("Bob")));
		person.insert("address", tx::JsonValue(address));

		std::string out;
		person.write(out);
		std::cout << "[Test 2 - Nested object]\n"
		          << out << "\n\n";
		// Expected: {"name":"Bob","address":{"city":"Toronto","zip":"M5V"}}
	}

	// --- Test 3: Array of primitives ---
	{
		tx::JsonArray arr;
		arr.push_back(tx::JsonValue(1));
		arr.push_back(tx::JsonValue(2));
		arr.push_back(tx::JsonValue(3));

		tx::JsonObject obj;
		obj.insert("nums", tx::JsonValue(arr));

		std::string out;
		obj.write(out);
		std::cout << "[Test 3 - Array of ints]\n"
		          << out << "\n\n";
		// Expected: {"nums":[1,2,3]}
	}

	// --- Test 4: Array of objects ---
	{
		tx::JsonObject a;
		a.insert("x", tx::JsonValue(1));
		tx::JsonObject b;
		b.insert("x", tx::JsonValue(2));

		tx::JsonArray arr;
		arr.push_back(tx::JsonValue(a));
		arr.push_back(tx::JsonValue(b));

		tx::JsonObject obj;
		obj.insert("points", tx::JsonValue(arr));

		std::string out;
		obj.write(out);
		std::cout << "[Test 4 - Array of objects]\n"
		          << out << "\n\n";
		// Expected: {"points":[{"x":1},{"x":2}]}
	}

	// --- Test 5: Empty object and empty array ---
	{
		tx::JsonObject obj;
		obj.insert("empty_obj", tx::JsonValue(tx::JsonObject{}));
		obj.insert("empty_arr", tx::JsonValue(tx::JsonArray{}));

		std::string out;
		obj.write(out);
		std::cout << "[Test 5 - Empty object and array]\n"
		          << out << "\n\n";
		// Expected: {"empty_obj":{},"empty_arr":[]}
	}

	// --- Test 6: Float formatting (should always have decimal point) ---
	{
		tx::JsonObject obj;
		obj.insert("whole_float", tx::JsonValue(2.0f));
		obj.insert("real_float", tx::JsonValue(3.14f));
		obj.insert("negative", tx::JsonValue(-1.5f));

		std::string out;
		obj.write(out);
		std::cout << "[Test 6 - Float formatting]\n"
		          << out << "\n\n";
		// Expected: whole_float gets ".0" appended, others as-is
	}

	// --- Test 7: String escaping edge cases ---
	{
		tx::JsonObject obj;
		obj.insert("empty_str", tx::JsonValue(std::string("")));
		obj.insert("spaces", tx::JsonValue(std::string("hello world")));
		obj.insert("symbols", tx::JsonValue(std::string("a/b&c=d")));

		std::string out;
		obj.write(out);
		std::cout << "[Test 7 - String values]\n"
		          << out << "\n\n";
	}

	// --- Test 8: Write to ostream vs write to string consistency ---
	{
		tx::JsonObject obj;
		obj.insert("key", tx::JsonValue(42));

		std::string str_out;
		obj.write(str_out);

		std::ostringstream oss;
		obj.write(oss);

		std::cout << "[Test 8 - ostream vs string consistency]\n";
		std::cout << "string: " << str_out << "\n";
		std::cout << "ostream: " << oss.str() << "\n";
		std::cout << "match: " << (str_out == oss.str() ? "YES" : "NO") << "\n\n";
	}

	// --- Test 9: Boolean values ---
	{
		tx::JsonObject obj;
		obj.insert("t", tx::JsonValue(true));
		obj.insert("f", tx::JsonValue(false));

		std::string out;
		obj.write(out);
		std::cout << "[Test 9 - Booleans]\n"
		          << out << "\n\n";
		// Expected: {"t":true,"f":false}
	}

	return 0;





	return 0;
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