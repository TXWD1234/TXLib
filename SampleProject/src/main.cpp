#include "Project.hpp"
#include "impl/hash_set.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/ring_buffer.hpp"
#include "tx/exception.hpp"
#include "tx/json.h"
#include <algorithm>
#include <concepts>
#include <random>
#include <string>
#include <string_view>
// #include "stb_image.hpp"
// #include <concepts>

// class Application {
// private:
// 	struct UpdateFunc {
// 		Application* ptr;
// 		inline void operator()() {
// 			ptr->update();
// 		}
// 	};
// 	struct RenderFunc {
// 		Application* ptr;
// 		inline void operator()() {
// 			ptr->render();
// 		}
// 	};
// 	tx::RE::Framework<re::Mode::FixTickRate | re::Mode::PrintFrameRate, UpdateFunc, RenderFunc> framework;

// 	bool initGLFW() {
// 		std::cout << "Initializing GLFW...\n";
// 		if (!glfwInit()) {
// 			std::cerr << "[FatalError]: Failed to init GLFW\n";
// 			return false;
// 		}
// 		return true;
// 	}
// 	bool initGLAD() {
// 		std::cout << "Initializing GLAD...\n";
// 		if (!gl::init((void*)glfwGetProcAddress)) {
// 			std::cerr << "[FatalError]: Failed to init GLAD\n";
// 			return false;
// 		}
// 		return true;
// 	}

// 	bool m_valid = 0;

// public:
// 	Application() {
// 		if (!initGLFW()) return;
// 		framework = decltype(framework){
// 			UpdateFunc{ this },
// 			RenderFunc{ this }
// 		};
// 		if (!framework.valid()) return;
// 		if (!initGLAD()) return;
// 		if (!init()) return;
// 		m_valid = 1;
// 	}
// 	~Application() {
// 		glfwTerminate();
// 	}

// 	void run() { this->framework.run(); }
// 	bool valid() const { return m_valid; }

// private:
// 	void onKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
// 		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
// 			switch (key) {
// 			}
// 		}
// 	}

// 	re::RE re;
// 	re::RSP rr;

// private:
// 	bool init() {
// 		tx::glfwSetKeyCallback<Application, &Application::onKeyEvent>(framework.getWindow(), this);
// 		tx::glBasicSettings();
// 		stbi_set_flip_vertically_on_load(true);
// 		glfwSwapInterval(0); // turn off vsync

// 		re.init();
// 		rr = re.createSectionProxy(re::readShaderSource("vertex.vert"), re::readShaderSource("fragment.frag"));


// 		int width, height, channels;
// 		std::vector<tx::u8*> data;
// 		data.reserve(11);

// 		for (int i = 1; i <= 12; i++) {
// 			if (i == 4) continue; // watch who ever is reading this code be so confusing...
// 			std::ostringstream oss;
// 			oss << "/home/TX_Jerry/Desktop/mpv-shot00" << std::setw(2) << std::setfill('0') << i << ".jpg";
// 			std::string path = oss.str();
// 			data.push_back(stbi_load(path.c_str(), &width, &height, &channels, 4));
// 			if (!data.back()) {
// 				std::cerr << "[Error]: stb_image failed to load image in frame: " << i << endl;
// 				return 0;
// 			}
// 		}
// 		tx::u32 length = width * height * 4;


// 		return 1;
// 	}

// 	tx::u32 frameCounter = 0;
// 	tx::u32 imageCount = 15;


// 	const float scaleIncreaseMult = 1.067f;
// 	const float scaleDecreaseMult = 0.9f;
// 	float scale = 0.1f, currentMult = scaleIncreaseMult;
// 	float degree = 0.0f, rotationSpeed = tx::ONE_DEGREE * -5.0f, degreeMax = 2 * tx::PI;
// 	tx::Rainbow colorEngine = tx::Rainbow(36);

// 	tx::u64 tickCounter = 0;
// 	void update() {
// 		// anim frame
// 		if (!(tickCounter % 3)) {
// 			frameCounter++;
// 			if (frameCounter >= 11) {
// 				frameCounter = 0;
// 			}
// 		}
// 		// scale
// 		scale *= currentMult;
// 		if (scale >= 2.0f) {
// 			currentMult = scaleDecreaseMult;
// 		} else if (scale <= 0.5f) {
// 			currentMult = scaleIncreaseMult;
// 		}
// 		// rotation
// 		degree += rotationSpeed;
// 		if (degree >= degreeMax) degree -= degreeMax;

// 		tickCounter++;
// 	}
// 	void render() {
// 		//rr.drawSprite(tx::Origin, anim.next(), tx::vec2{ 1.0f, 1.0f }, 0, colorEngine.getNextColor().compress());
// 		re.draw();
// 	}
// };



// int main() {


// 	return 0;
// 	std::cout << "Initializing Application...\n";
// 	Application app;
// 	if (!app.valid()) {
// 		std::cerr << "[FatalError]: Failed to init Application\n";
// 		return 1;
// 	}

// 	std::cout << "[Status]: Main Loop Starts\n";
// 	app.run();

// 	std::cout << "[Status]: Terminating...\n";
// 	return 0;
// }

// Overall Test <------------------------------------------------------------------------------

// ---- Test infrastructure ----

int g_passed = 0;
int g_failed = 0;

#define CHECK(cond, name)                             \
	do {                                              \
		if (cond) {                                   \
			std::cout << "  [PASS] " << name << "\n"; \
			++g_passed;                               \
		} else {                                      \
			std::cout << "  [FAIL] " << name << "\n"; \
			++g_failed;                               \
		}                                             \
	} while (0)

#define SECTION(name) std::cout << "\n== " << name << " ==\n"

// ---- Helpers ----

// Collect all values reachable by search into a sorted vector,
// used to verify tree contents match a reference set.
// Since we don't have iterators, we verify via repeated search.
void verify_contents(tx::AVLTree<int>& tree,
                     const std::vector<int>& expected,
                     const std::string& ctx) {
	for (int v : expected) {
		auto r = tree.search(v);
		if (!r || *r.value != v) {
			std::cout << "  [FAIL] " << ctx
			          << ": value " << v << " not found or wrong\n";
			++g_failed;
			return;
		}
	}
	++g_passed;
	std::cout << "  [PASS] " << ctx << ": all expected values present\n";
}

// ---- Test groups ----

void test_empty() {
	SECTION("Empty tree");
	tx::AVLTree<int> t;

	auto r = t.search(42);
	CHECK(!r && r.value == nullptr, "search on empty returns not-found");

	// remove on empty should not crash
	t.remove(42);
	CHECK(true, "remove on empty does not crash");
}

void test_single() {
	SECTION("Single element");
	tx::AVLTree<int> t;
	t.insert(10);

	auto r = t.search(10);
	CHECK(r && *r.value == 10, "find inserted single element");

	auto r2 = t.search(99);
	CHECK(!r2, "search miss on single-element tree");

	t.remove(10);
	auto r3 = t.search(10);
	CHECK(!r3, "element gone after remove");
}

void test_duplicate() {
	SECTION("Duplicate insert (ignore semantic)");
	tx::AVLTree<int> t;
	t.insert(5);
	t.insert(5); // should be silently ignored

	// still findable
	auto r = t.search(5);
	CHECK(r && *r.value == 5, "duplicate insert: value still present");

	// only one copy — remove once and it's gone
	t.remove(5);
	auto r2 = t.search(5);
	CHECK(!r2, "duplicate insert: only one copy existed");
}

void test_insert_search_ascending() {
	SECTION("Insert ascending (right-heavy, triggers left rotations)");
	tx::AVLTree<int> t;
	std::vector<int> vals = { 1, 2, 3, 4, 5, 6, 7 };
	for (int v : vals) t.insert(v);
	verify_contents(t, vals, "all values searchable after ascending insert");
}

void test_insert_search_descending() {
	SECTION("Insert descending (left-heavy, triggers right rotations)");
	tx::AVLTree<int> t;
	std::vector<int> vals = { 7, 6, 5, 4, 3, 2, 1 };
	for (int v : vals) t.insert(v);
	std::vector<int> sorted = { 1, 2, 3, 4, 5, 6, 7 };
	verify_contents(t, sorted, "all values searchable after descending insert");
}

void test_insert_search_zigzag() {
	SECTION("Insert zigzag (triggers double rotations)");
	tx::AVLTree<int> t;
	// Patterns that force LR and RL rotations
	std::vector<int> vals = { 10, 5, 8, 3, 7 };
	for (int v : vals) t.insert(v);
	verify_contents(t, vals, "zigzag insert: all values present");
}

void test_delete_leaf() {
	SECTION("Delete leaf node");
	tx::AVLTree<int> t;
	for (int v : { 5, 3, 7 }) t.insert(v);

	t.remove(3); // leaf
	CHECK(!t.search(3), "leaf 3 removed");
	CHECK(t.search(5), "root 5 still present");
	CHECK(t.search(7), "sibling 7 still present");
}

void test_delete_root_single() {
	SECTION("Delete root (only element)");
	tx::AVLTree<int> t;
	t.insert(42);
	t.remove(42);
	CHECK(!t.search(42), "root-only element removed");
	// insert after empty again
	t.insert(1);
	CHECK(t.search(1), "can insert into tree after it became empty");
}

void test_delete_root_with_children() {
	SECTION("Delete root with two children");
	tx::AVLTree<int> t;
	for (int v : { 5, 3, 7 }) t.insert(v);
	t.remove(5);
	CHECK(!t.search(5), "old root removed");
	CHECK(t.search(3), "left child still present");
	CHECK(t.search(7), "right child still present");
}

void test_delete_case3_left_child_only() {
	SECTION("Delete node with left child only (case 3)");
	// Build: insert 10, 5 — tree is right-heavy so 10 is root,
	// then 3 to make 5 have only a left child structure.
	// Actually easier: insert 5, 3. Node 5 has only left child 3.
	tx::AVLTree<int> t;
	t.insert(5);
	t.insert(3);
	// At this point 5 is root with left child 3
	t.remove(5);
	CHECK(!t.search(5), "node with left-child-only removed");
	CHECK(t.search(3), "left child promoted");
}

void test_delete_case4_right_child_only() {
	SECTION("Delete node with right child only (case 4)");
	tx::AVLTree<int> t;
	t.insert(5);
	t.insert(7);
	// 5 is root, right child 7
	t.remove(5);
	CHECK(!t.search(5), "node with right-child-only removed");
	CHECK(t.search(7), "right child promoted");
}

void test_delete_triggers_rebalance() {
	SECTION("Delete triggers rebalance");
	tx::AVLTree<int> t;
	// Insert enough to guarantee a rotation is needed on delete
	// Tree: 4 is root, left subtree 2(1,3), right subtree 6(5,7)
	for (int v : { 4, 2, 6, 1, 3, 5, 7 }) t.insert(v);
	t.remove(1); // triggers rebalance on left side
	std::vector<int> remaining = { 2, 3, 4, 5, 6, 7 };
	verify_contents(t, remaining, "tree consistent after rebalance on delete");
}

void test_remove_nonexistent() {
	SECTION("Remove nonexistent element");
	tx::AVLTree<int> t;
	for (int v : { 1, 2, 3 }) t.insert(v);
	t.remove(99); // should not crash or corrupt
	verify_contents(t, { 1, 2, 3 }, "tree intact after remove of missing element");
}

void test_pointer_stability_warning() {
	SECTION("FindResult pointer note");
	// FindResult::value points into m_data which can be reallocated on insert.
	// This test just documents the contract — do NOT hold the pointer across inserts.
	tx::AVLTree<int> t;
	t.insert(10);
	auto r = t.search(10);
	CHECK(r && *r.value == 10, "pointer valid immediately after search");
	// (Do not use r.value after further inserts — that would be UB)
}

void test_stress_vs_std_set() {
	SECTION("Stress test vs std::set (1000 random ops)");

	tx::AVLTree<int> tree;
	std::set<int> ref;

	std::mt19937 rng(42);
	std::uniform_int_distribution<int> val_dist(0, 199);
	std::uniform_int_distribution<int> op_dist(0, 2); // 0=insert,1=remove,2=search

	bool ok = true;
	for (int i = 0; i < 1000; ++i) {
		cout << i << endl;
		int v = val_dist(rng);
		int op = op_dist(rng);

		if (op == 0) {
			tree.insert(v);
			ref.insert(v);
		} else if (op == 1) {
			tree.remove(v);
			ref.erase(v);
		} else {
			bool tree_found = (bool)tree.search(v);
			bool ref_found = ref.count(v) > 0;
			if (tree_found != ref_found) {
				std::cout << "  MISMATCH at v=" << v
				          << " tree=" << tree_found
				          << " ref=" << ref_found << "\n";
				ok = false;
			}
		}
	}

	// Final: verify every ref element is in tree and vice-versa
	for (int v : ref) {
		if (!tree.search(v)) {
			std::cout << "  MISSING in tree: " << v << "\n";
			ok = false;
		}
	}
	// Check nothing extra in tree (spot-check values not in ref)
	for (int v = 0; v <= 199; ++v) {
		bool in_ref = ref.count(v) > 0;
		bool in_tree = (bool)tree.search(v);
		if (in_ref != in_tree) {
			std::cout << "  DISCREPANCY at v=" << v << "\n";
			ok = false;
		}
	}

	CHECK(ok, "1000 random insert/remove/search ops match std::set");
}

void test_stress_delete_all() {
	SECTION("Insert N then delete all in random order");
	tx::AVLTree<int> tree;
	std::vector<int> vals;
	for (int i = 0; i < 50; ++i) vals.push_back(i);
	int counter = 0;
	for (int v : vals) {
		std::cout << "inserting " << counter << '\n';
		tree.insert(v);
		counter++;
		std::cout << '\n';
	}
	counter = 0;

	std::cout << "End Inserting ----------------------------------------------------\n"
	          << "Final Tree View:\n";
	//tree.printAll();

	std::mt19937 rng(7);
	std::shuffle(vals.begin(), vals.end(), rng);

	bool ok = true;
	for (int v : vals) {
		std::cout << counter << "th deletion, deleting value: " << v << endl;
		tree.remove(v);
		// tree.printTreeRecursive(21);
		// std::cout << "**************************************************************\n"
		//           << "**************************************************************\n";
		//tree.printTreeRecursive(29);
		if (tree.search(v)) {
			ok = false;
			break;
		}
		counter++;
		std::cout << '\n';
	}
	CHECK(ok, "all 50 elements removable in random order");

	// tree should be empty — inserting fresh should work
	tree.insert(999);
	CHECK(tree.search(999), "tree usable after being emptied");
}

void test_custom_comparator() {
	SECTION("Custom comparator (reverse/descending order)");
	tx::AVLTree<int, std::greater<int>> t(std::greater<int>{});
	for (int v : { 3, 1, 4, 1, 5, 9, 2, 6 }) t.insert(v);

	// All unique values should still be findable regardless of order
	for (int v : { 1, 2, 3, 4, 5, 6, 9 }) {
		if (!t.search(v)) {
			CHECK(false, "reverse-order tree missing value");
			return;
		}
	}
	CHECK(true, "reverse-order tree: all values found");

	t.remove(5);
	CHECK(!t.search(5), "reverse-order tree: remove works");
}

// ---- Main ----

template class tx::HashSetOverlay<int>;
template class tx::HashSetOverlay<std::string>;

struct Word {
	std::string eng;
	std::string chn = "";
};

template <std::invocable<int, int> Func>
void idk(Func&& func, int n) {
	for (int c = 1; c < n; c++) {
		int left = 2 * n + c - c * c;
		int f = left / (c * 2);
		if (f * c * 2 == left && f > 0) {
			func(c, f);
		}
	}
}

int main() {

	std::cout << "Hello"
	             " World";

	return 0;

	int i = 3;
	while (true) {
		float doubled = (double)(i * i * i) / 3.0 * 2.0;
		if (tx::isInt(std::sqrt(doubled))) {
			std::print("i = {}, i^3 = {}, sqrt(i^3/3*2) = {}, ans = {}",
			           i, i * i * i, sqrt(doubled),
			           doubled / 2.0);
			cin.get();
		}
		i += 3;
	}

	return 0;

	int result = 0;
	int resultCount = 0;
	for (int i = 0; i < 1000000; i++) {
		int count = 0;
		idk([&](int c, int f) {
			count++;
		},
		    i);
		if (count > resultCount) {
			result = i;
			resultCount = count;
		}
	}


	std::print("result = {}; resultCount = {}\n", result, resultCount);
	idk([](int c, int f) {
		std::print("c: {}, f: {}\n", c, f);
	},
	    result);


	return 0;

	tx::u32 count;
	std::cout << "请输入本次学习的单词数量：\n";
	std::cin >> count;
	std::cout << "请输入本次学习的单词（单词之间用空格分开）：\n";

	std::vector<Word> data;
	for (tx::u32 i = 0; i < count; i++) {
		std::string eng;
		std::cin >> eng;
		data.push_back(Word(eng));
	}

	std::cout << "请输入本次学习的单词对应的中文（按英文输入的顺序）：\n";
	for (tx::u32 i = 0; i < count; i++) {
		std::string chn;
		std::cin >> chn;
		data[i].chn = chn;
	}

	std::cout << "\n\n开始复习。请输入以下中文对应的单词：\n\n";

	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<bool> dist(false, true);
	std::shuffle(data.begin(), data.end(), rng);

	while (!data.empty()) {
		for (tx::u32 i = 0; i < data.size(); i++) {
			Word& word = data[i];
			std::cout << word.chn << '\n';
			std::string ans;
			std::cin >> ans;
			if (ans == word.eng) {
				std::cout << "正确。\n";
				data.erase(data.begin() + i);
				i--;
			} else {
				std::cout << "错误。正确答案：" << word.eng;
			}
			std::cin.ignore();
			std::cin.get();
			std::cout << "\033[2J\033[H";
		}
	}

	std::cout << "恭喜完成今天任务！\n";



	return 0;

	//tx::impl::assert_impl(tx::impl::preset_out_of_range(0, 0));

	//tx::impl::assert::buffer_empty(0, 0);

	tx::RingBufferOverlay<int> a;

	std::string str = "runtime";
	tx::impl::assert_impl([] { return true; }, "compile time");
	tx::impl::assert_impl([] { return true; }, [&] { return str; });
	//tx::impl::assert_impl([] { return true; }, str);

	return 0;

	//tx::JsonDocument a;
	//tx::JsonParser parser("");
	//parser.parse();


	std::cout << "AVLTree Test Suite\n";
	std::cout << "==================\n";

	// test_empty();
	// test_single();
	// test_duplicate();
	// test_insert_search_ascending();
	// test_insert_search_descending();

	// test_insert_search_zigzag();

	// test_delete_leaf();
	// test_delete_root_single();
	// test_delete_root_with_children();
	// test_delete_case3_left_child_only();
	// test_delete_case4_right_child_only();
	// test_delete_triggers_rebalance();
	// test_remove_nonexistent();
	// test_pointer_stability_warning();

	// test_stress_vs_std_set();

	test_stress_delete_all();
	test_custom_comparator();

	std::cout << "\n==================\n";
	std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
	return g_failed == 0 ? 0 : 1;
}




// Insertion Test <----------------------------------------------------------------------------

// // ---- Infra ----

// int g_passed = 0;
// int g_failed = 0;

// #define CHECK(cond, name)                               \
// 	do {                                                \
// 		if (cond) {                                     \
// 			std::cout << "  [PASS] " << (name) << "\n"; \
// 			++g_passed;                                 \
// 		} else {                                        \
// 			std::cout << "  [FAIL] " << (name) << "\n"; \
// 			++g_failed;                                 \
// 		}                                               \
// 	} while (0)

// #define SECTION(name) std::cout << "\n== " << (name) << " ==\n"

// // ---- Helpers ----

// // Verify every value in `ref` is found in `tree` with correct value,
// // and every value NOT in `ref` (sampled from universe) is not found.
// bool verify_against_set(tx::AVLTree<int>& tree,
//                         const std::set<int>& ref,
//                         int universe_lo, int universe_hi,
//                         std::string& failure_msg) {
// 	// Check all ref values are present
// 	for (int v : ref) {
// 		auto r = tree.search(v);
// 		if (!r || *r.value != v) {
// 			failure_msg = "value " + std::to_string(v) + " in ref but not found in tree";
// 			return false;
// 		}
// 	}
// 	// Check all values outside ref are absent
// 	for (int v = universe_lo; v <= universe_hi; ++v) {
// 		bool in_ref = ref.count(v) > 0;
// 		bool in_tree = (bool)tree.search(v);
// 		if (in_ref != in_tree) {
// 			failure_msg = "value " + std::to_string(v) +
// 			              " in_ref=" + std::to_string(in_ref) +
// 			              " in_tree=" + std::to_string(in_tree);
// 			return false;
// 		}
// 	}
// 	return true;
// }

// // ---- Tests ----

// void test_ascending() {
// 	SECTION("Ascending insert (1..64)");
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	for (int i = 1; i <= 64; ++i) {
// 		tree.insert(i);
// 		ref.insert(i);
// 	}
// 	std::string msg;
// 	CHECK(verify_against_set(tree, ref, 0, 65, msg), "ascending: " + msg);
// }

// void test_descending() {
// 	SECTION("Descending insert (64..1)");
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	for (int i = 64; i >= 1; --i) {
// 		tree.insert(i);
// 		ref.insert(i);
// 	}
// 	std::string msg;
// 	CHECK(verify_against_set(tree, ref, 0, 65, msg), "descending: " + msg);
// }

// void test_alternating() {
// 	SECTION("Alternating insert (forces zigzag rotations)");
// 	// 1,64,2,63,3,62,... forces constant LR/RL double rotations
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	int lo = 1, hi = 64;
// 	while (lo <= hi) {
// 		tree.insert(lo);
// 		ref.insert(lo++);
// 		if (lo <= hi) {
// 			tree.insert(hi);
// 			ref.insert(hi--);
// 		}
// 	}
// 	std::string msg;
// 	CHECK(verify_against_set(tree, ref, 0, 65, msg), "alternating: " + msg);
// }

// void test_duplicates() {
// 	SECTION("Duplicate inserts ignored");
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	for (int i = 1; i <= 20; ++i) {
// 		tree.insert(i);
// 		ref.insert(i);
// 		tree.insert(i); // duplicate — should be ignored
// 	}
// 	std::string msg;
// 	CHECK(verify_against_set(tree, ref, 0, 21, msg), "duplicates: " + msg);
// }

// void test_incremental_verify() {
// 	SECTION("Incremental verify after each insert (ascending)");
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	bool ok = true;
// 	for (int i = 1; i <= 100; ++i) {
// 		tree.insert(i);
// 		ref.insert(i);
// 		// after every insert, verify entire universe so far
// 		for (int v = 0; v <= 101; ++v) {
// 			bool in_ref = ref.count(v) > 0;
// 			bool in_tree = (bool)tree.search(v);
// 			if (in_ref != in_tree) {
// 				std::cout << "  MISMATCH after inserting " << i
// 				          << ": v=" << v
// 				          << " in_ref=" << in_ref
// 				          << " in_tree=" << in_tree << "\n";
// 				ok = false;
// 				goto done_inc_asc;
// 			}
// 		}
// 	}
// done_inc_asc:
// 	CHECK(ok, "incremental verify ascending");
// }

// void test_incremental_verify_descending() {
// 	SECTION("Incremental verify after each insert (descending)");
// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	bool ok = true;
// 	for (int i = 100; i >= 1; --i) {
// 		tree.insert(i);
// 		ref.insert(i);
// 		for (int v = 0; v <= 101; ++v) {
// 			bool in_ref = ref.count(v) > 0;
// 			bool in_tree = (bool)tree.search(v);
// 			if (in_ref != in_tree) {
// 				std::cout << "  MISMATCH after inserting " << i
// 				          << ": v=" << v
// 				          << " in_ref=" << in_ref
// 				          << " in_tree=" << in_tree << "\n";
// 				ok = false;
// 				goto done_inc_desc;
// 			}
// 		}
// 	}
// done_inc_desc:
// 	CHECK(ok, "incremental verify descending");
// }

// void test_stress_random(uint32_t seed, int n_ops, int universe) {
// 	std::string name = "random seed=" + std::to_string(seed) +
// 	                   " n=" + std::to_string(n_ops) +
// 	                   " universe=" + std::to_string(universe);
// 	SECTION(name);

// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	std::mt19937 rng(seed);
// 	std::uniform_int_distribution<int> dist(0, universe - 1);

// 	for (int i = 0; i < n_ops; ++i) {
// 		int v = dist(rng);
// 		tree.insert(v);
// 		ref.insert(v);
// 	}

// 	std::string msg;
// 	bool ok = verify_against_set(tree, ref, 0, universe, msg);
// 	CHECK(ok, name + (ok ? "" : ": " + msg));
// }

// void test_stress_incremental_random(uint32_t seed, int n_ops, int universe) {
// 	std::string name = "incremental random seed=" + std::to_string(seed);
// 	SECTION(name);

// 	tx::AVLTree<int> tree;
// 	std::set<int> ref;
// 	std::mt19937 rng(seed);
// 	std::uniform_int_distribution<int> dist(0, universe - 1);

// 	bool ok = true;
// 	for (int i = 0; i < n_ops; ++i) {
// 		int v = dist(rng);
// 		tree.insert(v);
// 		ref.insert(v);

// 		// full verify every 10 inserts to catch corruption early
// 		if (i % 10 == 0) {
// 			for (int u = 0; u < universe; ++u) {
// 				bool in_ref = ref.count(u) > 0;
// 				bool in_tree = (bool)tree.search(u);
// 				if (in_ref != in_tree) {
// 					std::cout << "  MISMATCH at op " << i
// 					          << " inserted=" << v
// 					          << " checking u=" << u
// 					          << " in_ref=" << in_ref
// 					          << " in_tree=" << in_tree << "\n";
// 					ok = false;
// 					goto done;
// 				}
// 			}
// 		}
// 	}
// done:
// 	CHECK(ok, name);
// }

// // ---- Main ----

// int main() {
// 	std::cout << "AVLTree Insertion & Search Stress Test\n";
// 	std::cout << "=======================================\n";

// 	test_ascending();
// 	test_descending();
// 	test_alternating();
// 	test_duplicates();
// 	test_incremental_verify();
// 	test_incremental_verify_descending();

// 	// varied seeds and sizes
// 	test_stress_random(42, 1000, 200);
// 	test_stress_random(123, 1000, 50); // high collision rate
// 	test_stress_random(999, 5000, 1000);
// 	test_stress_random(7, 500, 10); // very small universe, many duplicates

// 	test_stress_incremental_random(42, 500, 100);
// 	test_stress_incremental_random(777, 500, 30); // tiny universe

// 	std::cout << "\n=======================================\n";
// 	std::cout << "Results: " << g_passed << " passed, "
// 	          << g_failed << " failed\n";
// 	return g_failed == 0 ? 0 : 1;
// }


// Deletion Test <-----------------------------------------------------------------------------

// // ---- Infra ----

// int g_passed = 0;
// int g_failed = 0;

// #define CHECK(cond, name)                               \
// 	do {                                                \
// 		if (cond) {                                     \
// 			std::cout << "  [PASS] " << (name) << "\n"; \
// 			++g_passed;                                 \
// 		} else {                                        \
// 			std::cout << "  [FAIL] " << (name) << "\n"; \
// 			++g_failed;                                 \
// 		}                                               \
// 	} while (0)

// #define SECTION(name) std::cout << "\n== " << (name) << " ==\n"

// bool verify(tx::AVLTree<int>& tree, const std::set<int>& ref,
//             int lo, int hi, std::string& msg) {
// 	for (int v = lo; v <= hi; ++v) {
// 		bool in_ref = ref.count(v) > 0;
// 		bool in_tree = (bool)tree.search(v);
// 		if (in_ref != in_tree) {
// 			msg = "v=" + std::to_string(v) +
// 			      " in_ref=" + std::to_string(in_ref) +
// 			      " in_tree=" + std::to_string(in_tree);
// 			return false;
// 		}
// 	}
// 	return true;
// }

// // ---- Case 1: target is a leaf ----
// //
// // Build the simplest possible tree with a leaf and delete it.
// // Also test leaf-is-root, leaf-is-left-child, leaf-is-right-child,
// // and that deletion triggers propagation correctly.
// //
// //      5
// //     / \
// //    3   7
// //
// // Delete 3 (left leaf), then 7 (right leaf), then 5 (root-as-leaf).
// //
// // Also test a deeper leaf deletion that requires propagation:
// //
// //        4
// //      /   \
// //     2     6
// //    / \   / \
// //   1   3 5   7
// //
// // Delete 1 — leaf, triggers propagation up through 2 then 4.

// void test_case1_leaf() {
// 	SECTION("Case 1: delete leaf");

// 	// --- subcase: left leaf ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 5, 3, 7 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(3);
// 		ref.erase(3);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 10, msg);
// 		CHECK(ok, "case1 left leaf: " + (ok ? "tree intact" : msg));
// 	}

// 	// --- subcase: right leaf ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 5, 3, 7 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(7);
// 		ref.erase(7);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 10, msg);
// 		CHECK(ok, "case1 right leaf: " + (ok ? "tree intact" : msg));
// 	}

// 	// --- subcase: root is only node ---
// 	{
// 		tx::AVLTree<int> t;
// 		t.insert(42);
// 		t.remove(42);
// 		std::string msg = "root-only removal";
// 		CHECK(!t.search(42), "case1 root-only: removed");
// 		t.insert(1);
// 		CHECK((bool)t.search(1), "case1 root-only: re-insert after empty works");
// 	}

// 	// --- subcase: leaf deletion triggers propagation ---
// 	// Tree after inserting 1..7 in order 4,2,6,1,3,5,7:
// 	//        4
// 	//      /   \
//     //     2     6
// 	//    / \   / \
//     //   1   3 5   7
// 	// Deleting 1: leaf, propagates up, bf of 2 becomes 0 then stops (height unchanged? no —
// 	// 2 goes from bf=0 to bf=-1, height unchanged, propagation stops. Correct.
// 	// Then deleting 3: leaf, 2 now has no children — bf goes to 0, height decreases,
// 	// propagation continues up to 4.
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 4, 2, 6, 1, 3, 5, 7 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(1);
// 		ref.erase(1);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 8, msg);
// 		CHECK(ok, "case1 propagation step1: " + (ok ? "ok" : msg));

// 		t.remove(3);
// 		ref.erase(3);
// 		ok = verify(t, ref, 0, 8, msg);
// 		CHECK(ok, "case1 propagation step2 (triggers rebalance): " + (ok ? "ok" : msg));
// 	}
// }

// // ---- Case 2: swapper's parent is not target ----
// //
// // findClosest goes right then left as far as possible.
// // For swapper's parent to NOT be target, the right subtree must have
// // a left child — i.e., the in-order successor is not the immediate right child.
// //
// // Build:
// //        8
// //       / \
// //      4   12
// //     / \  / \
// //    2   6 10  14
// //           \
// //           11  <- not needed, let's use minimal
// //
// // Actually the minimal tree where swapper's parent != target:
// //
// //      5
// //     / \
// //    2   8
// //       / \
// //      6   9
// //
// // Delete 5: findClosest goes right to 8, then left to 6. Swapper=6, swapper's parent=8 != 5.
// // After deletion, 6 takes 5's place, 8 loses its left child.
// //
// // Also test a case where swapper has a right child of its own:
// //
// //      5
// //     / \
// //    2   9
// //       / \
// //      7   11
// //       \
// //        8   <- swapper=7, has right child 8
// //
// // Delete 5: swapper=7 (leftmost of right subtree), swapper has right child 8.

// void test_case2_swapper_not_child_of_target() {
// 	SECTION("Case 2: swapper parent != target");

// 	// --- subcase: swapper is a leaf ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		// insert to get: 5 root, left=2, right=8, 8's left=6, 8's right=9
// 		// Insert order matters for AVL shape. Use: 5,2,8,6,9
// 		// After 5,2,8: balanced, bf=0 for all
// 		// After 6: goes under 8's left. 8's bf=1, 5's bf=-1
// 		// After 9: goes under 8's right. 8's bf=0, 5's bf=-1...
// 		// Actually let's just insert and trust the tree, then verify case via deletion result.
// 		for (int v : { 5, 2, 8, 6, 9 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(5);
// 		ref.erase(5);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 15, msg);
// 		CHECK(ok, "case2 swapper-is-leaf: " + (ok ? "ok" : msg));
// 		// 6 should now be in tree (was swapper)
// 		CHECK((bool)t.search(6), "case2 swapper-is-leaf: swapper value still searchable");
// 	}

// 	// --- subcase: swapper has right child ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		// Want: delete a node whose in-order successor has a right child.
// 		// Insert: 10, 5, 15, 12, 18, 11, 13
// 		//         10
// 		//        /  \
//         //       5    15
// 		//           /  \
//         //          12   18
// 		//         /  \
//         //        11   13
// 		// Delete 10: in-order successor = 11 (leftmost of right subtree).
// 		// swapper=11, swapper parent=12 != 10. swapper has no right child here.
// 		// Let's instead do:
// 		// Insert: 10, 5, 15, 12, 18, 13
// 		//         10
// 		//        /  \
//         //       5    15
// 		//           /  \
//         //          12   18
// 		//            \
//         //            13   <- swapper=12 going left... no, findClosest goes right then left.
// 		// findClosest(10): go right to 15, then left to 12, then left = null. swapper=12.
// 		// swapper=12 has right child 13. swapper parent=15 != 10. This is case 2.
// 		for (int v : { 10, 5, 15, 12, 18, 13 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(10);
// 		ref.erase(10);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 20, msg);
// 		CHECK(ok, "case2 swapper-has-right-child: " + (ok ? "ok" : msg));
// 	}

// 	// --- subcase: multiple case2 deletions in sequence ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 20, 10, 30, 25, 35, 22, 27 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		// Delete root twice in a row, both should hit case 2
// 		t.remove(20);
// 		ref.erase(20);
// 		t.remove(22);
// 		ref.erase(22);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 40, msg);
// 		CHECK(ok, "case2 sequential: " + (ok ? "ok" : msg));
// 	}
// }

// // ---- Case 3: swapper is left child of target ----
// //
// // findClosest returns node's left child directly when node has NO right child.
// // So we need a node with only a left child.
// // In a balanced AVL tree, a node with only a left child must have that child
// // as a leaf (otherwise it would be imbalanced).
// //
// // Simplest: insert 2 nodes so root has only a left child.
// //      3
// //     /
// //    2     <- delete 3, swapper=2 (left child, no right child on target)
// //
// // More complex: build a tree where an internal node has only a left child leaf.
// //
// //        4
// //       / \
// //      2   6
// //     /     \
// //    1       7
// //
// // This tree has bf=-1 for node 2 (left child only) and bf=1 for 6.
// // Wait, that tree: insert 4,2,6,1,7
// // After 4,2,6: bf=0
// // After 1: 2's bf=1, 4's bf=1
// // After 7: 6's bf=-1, 4's bf=0 (balanced)
// // Delete 6... 6 has only right child 7, that's case 4.
// // Delete 2: 2 has only left child 1, that's case 3.

// void test_case3_swapper_is_left_child() {
// 	SECTION("Case 3: swapper is left child of target");

// 	// --- subcase: target is root, left child only ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		// Insert descending pair so root has only left child
// 		// Insert 3, then 2: 3 is root, 2 is left child
// 		for (int v : { 3, 2 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(3);
// 		ref.erase(3);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 5, msg);
// 		CHECK(ok, "case3 root-left-only: " + (ok ? "ok" : msg));
// 		CHECK((bool)t.search(2), "case3 root-left-only: left child promoted to root");
// 	}

// 	// --- subcase: target is internal node with left child only ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 4, 2, 6, 1, 7 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		// 2 has only left child 1 here (see analysis above)
// 		t.remove(2);
// 		ref.erase(2);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 8, msg);
// 		CHECK(ok, "case3 internal-left-only: " + (ok ? "ok" : msg));
// 	}

// 	// --- subcase: deletion triggers propagation up ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		// Build a taller tree so propagation has to climb
// 		for (int v : { 8, 4, 12, 2, 6, 10, 14, 1, 3 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		// Delete 2: has left child 1 only (case 3), then propagation climbs
// 		// Actually let's delete a node we're sure has only a left child
// 		// by first deleting its right subtree
// 		t.remove(3);
// 		ref.erase(3); // make 2 have only left child
// 		t.remove(2);
// 		ref.erase(2); // now case 3: 2 has only left child 1
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 16, msg);
// 		CHECK(ok, "case3 with-propagation: " + (ok ? "ok" : msg));
// 	}

// 	// --- subcase: sequential case3 deletions ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 5, 3, 7, 2, 6 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		t.remove(6);
// 		ref.erase(6); // make 7 have only left... actually 7 has no children
// 		// Let's just do a sequence and verify at each step
// 		bool ok = true;
// 		std::string msg;
// 		for (int v : { 7, 5, 3 }) {
// 			t.remove(v);
// 			ref.erase(v);
// 			if (!verify(t, ref, 0, 10, msg)) {
// 				ok = false;
// 				break;
// 			}
// 		}
// 		CHECK(ok, "case3 sequential: " + (ok ? "ok" : msg));
// 	}
// }

// // ---- Case 4: swapper is right child of target ----
// //
// // findClosest goes right, then tries to go left — if right child has no left child,
// // swapper IS the right child. This means target has a right child with no left child.
// //
// // Simplest:
// //      3
// //       \
// //        4    <- delete 3, swapper=4 (right child, no left child on swapper)
// //
// // More interesting: right child of target has its own right child.
// //      3
// //       \
// //        5
// //         \
// //          6   <- swapper=5, has right child 6
// //
// // But that would be imbalanced. So we need a proper balanced tree.
// //
// // Insert 3,5: 3 is root, 5 is right child. Delete 3 -> case 4, swapper=5 leaf.
// //
// // For swapper with right child:
// // Insert 4,2,6,5,7 ->
// //      4
// //     / \
// //    2   6
// //       / \
// //      5   7
// // Delete 4: findClosest(4) -> right=6, left of 6 = 5. So swapper=5, parent=6 != 4. That's case 2.
// //
// // We need swapper to BE the right child. So right child must have no left child.
// // Insert 4,2,6,7:
// //      4
// //     / \
// //    2   6
// //         \
// //          7
// // Delete 4: findClosest(4) -> right=6, left of 6=null. swapper=6. swapper parent=4. Case 4.
// // swapper(6) has right child 7.

// void test_case4_swapper_is_right_child() {
// 	SECTION("Case 4: swapper is right child of target");

// 	// --- subcase: swapper is leaf ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 3, 5 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(3);
// 		ref.erase(3);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 10, msg);
// 		CHECK(ok, "case4 swapper-leaf: " + (ok ? "ok" : msg));
// 		CHECK((bool)t.search(5), "case4 swapper-leaf: right child promoted");
// 	}

// 	// --- subcase: swapper has right child, target has left child ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 4, 2, 6, 7 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		// Tree: 4(root), left=2, right=6, 6's right=7
// 		// Delete 4: case 4, swapper=6 (right child of 4, no left child)
// 		// swapper has right child 7. Target has left child 2.
// 		t.remove(4);
// 		ref.erase(4);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 10, msg);
// 		CHECK(ok, "case4 swapper-has-right-child: " + (ok ? "ok" : msg));
// 	}

// 	// --- subcase: target is root, swapper becomes new root ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 5, 8 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}

// 		t.remove(5);
// 		ref.erase(5);
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 15, msg);
// 		CHECK(ok, "case4 target-is-root: " + (ok ? "ok" : msg));
// 		CHECK((bool)t.search(8), "case4 target-is-root: swapper is new root");
// 	}

// 	// --- subcase: deletion triggers rebalance (bf goes to -2 after case4) ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		// Build tree where case4 deletion creates imbalance
// 		//        6
// 		//       / \
//         //      4   8
// 		//     /     \
//         //    2        10
// 		//              \
//         //               12  <- too tall, AVL won't allow this without left sibling
// 		// Let's just use a known sequence and verify
// 		for (int v : { 6, 4, 8, 2, 10, 9 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		t.remove(4);
// 		ref.erase(4); // forces rebalance after case4 or case3
// 		std::string msg;
// 		bool ok = verify(t, ref, 0, 15, msg);
// 		CHECK(ok, "case4 triggers-rebalance: " + (ok ? "ok" : msg));
// 	}

// 	// --- subcase: sequential case4 deletions ---
// 	{
// 		tx::AVLTree<int> t;
// 		std::set<int> ref;
// 		for (int v : { 1, 3, 5, 7, 9 }) {
// 			t.insert(v);
// 			ref.insert(v);
// 		}
// 		bool ok = true;
// 		std::string msg;
// 		// Delete in ascending order — each time the target tends to have
// 		// only a right child or right-leaning successor
// 		for (int v : { 1, 3, 5 }) {
// 			t.remove(v);
// 			ref.erase(v);
// 			if (!verify(t, ref, 0, 12, msg)) {
// 				ok = false;
// 				break;
// 			}
// 		}
// 		CHECK(ok, "case4 sequential: " + (ok ? "ok" : msg));
// 	}
// }

// // ---- Main ----

// int main() {
// 	std::cout << "AVLTree Deletion Case Tests\n";
// 	std::cout << "===========================\n";

// 	test_case1_leaf();
// 	test_case2_swapper_not_child_of_target();
// 	test_case3_swapper_is_left_child();
// 	test_case4_swapper_is_right_child();

// 	std::cout << "\n===========================\n";
// 	std::cout << "Results: " << g_passed << " passed, "
// 	          << g_failed << " failed\n";
// 	return g_failed == 0 ? 0 : 1;
// }
