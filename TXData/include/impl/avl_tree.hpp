// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/math.h"
#include "tx/type_traits.hpp"
#include <vector>
#include <array>
#include <concepts>

namespace tx {

/**
 * Iterators:
 * Iterators are provided, but not encouraged to use.
 * They exist purely to match the STL `find()` and `end()` pattern
 * The encouraged way to search is to use the `search()` function
 * For the purpose of traversing / iterating, use `foreach()`
 */
template <class T, invocable_r<bool, const T&, const T&> CmpFunc = std::less<>>
class AVLTree {
	/**
	 * Everytime an operation finishes, the tree is garanteed to be balanced.
	 * Therefore any operation can just assume the tree is balanced.
	 */

	/**
	 * Assume cmp is std::less or same semantic (same if reversed, just the order will be reversed too)
	 */

	/**
	 * NOTE: Rotation naming is inverted from convention.
	 * dir == Right means the moving-up node is on the right (right rotation fixes right-heavy imbalance).
	 * dir == Left means the moving-up node is on the left (left rotation fixes left-heavy imbalance).
	 */
public:
	using value_type = T;

	struct FindResult {
		T* value = nullptr; // Points directly into your flat data vector
		bool found = false;

		// Allows clean conditional checking
		explicit operator bool() const { return found; }
	};

public:
	AVLTree(CmpFunc&& cmpFunc = std::less<>{}) : m_cmp(cmpFunc) {}



	// AVL Tree Test Suite
	// Tests for tx::AVLTree
	// Since AVLTree doesn't yet have a public search() or traversal,
	// this file assumes you've added the following test hooks to the class
	// (shown below as a guide — paste into AVLTree's public section):
	// ============================================================
	// --- TEST HOOKS (can be removed for release builds) ---
	// Returns pointer to value if found, nullptr otherwise.
	const T* search(const T& val) const {
		if (m_nodes.empty()) return nullptr;
		u32 curIndex = m_meta.root;
		while (true) {
			const Node_impl& cur = m_nodes[curIndex];
			if (m_cmp(val, m_data[cur.data]))
				curIndex = cur.children[Left];
			else if (m_cmp(m_data[cur.data], val))
				curIndex = cur.children[Right];
			else
				return &m_data[cur.data];
			if (isNull(curIndex)) return nullptr;
		}
	}
	struct ValidationResult {
		bool valid = true;
		std::string error;
	};
	// Walks the whole tree and checks:
	//   1. BF stored == BF recomputed from subtree heights
	//   2. |BF| <= 1 everywhere
	//   3. BST ordering property holds
	//   4. Parent links are consistent
	ValidationResult validate() const {
		if (isNull(m_meta.root)) return {};
		ValidationResult res;
		validate_impl(m_meta.root, res);
		return res;
	}

private:
	// Returns height of subtree rooted at index, or -1 for null.
	int validate_impl(u32 index, ValidationResult& res) const {
		if (isNull(index)) return -1;
		const Node_impl& node = m_nodes[index];
		// check parent link
		if (!isNull(node.parent)) {
			const Node_impl& par = m_nodes[node.parent];
			if (par.children[0] != index && par.children[1] != index) {
				res.valid = false;
				res.error = "Parent link broken at node " + std::to_string(index);
			}
		}
		int lh = validate_impl(node.children[Left], res);
		int rh = validate_impl(node.children[Right], res);
		// check BST order
		if (!isNull(node.children[Left])) {
			if (!m_cmp(m_data[m_nodes[node.children[Left]].data], m_data[node.data])) {
				res.valid = false;
				res.error = "BST violation: left child >= parent at node " + std::to_string(index);
			}
		}
		if (!isNull(node.children[Right])) {
			if (!m_cmp(m_data[node.data], m_data[m_nodes[node.children[Right]].data])) {
				res.valid = false;
				res.error = "BST violation: parent >= right child at node " + std::to_string(index);
			}
		}
		// check BF
		i8 computedBf = static_cast<i8>(lh - rh); // adjust sign to match your convention
		i8 storedBf = m_nodemeta[index].bf;
		if (computedBf != storedBf) {
			res.valid = false;
			res.error = "BF mismatch at node " + std::to_string(index) + ": stored=" + std::to_string(storedBf) + " computed=" + std::to_string(computedBf);
		}
		if (std::abs(storedBf) > 1) {
			res.valid = false;
			res.error = "Balance violation at node " + std::to_string(index) + ": bf=" + std::to_string(storedBf);
		}
		return 1 + std::max(lh, rh);
	}
	// == == == == == == == == == == == == == == == == == == == == == == == == == == == == == ==
public:
	void insert(const T& val) {
		bool found = false;
		u32 parentIndex = search_impl(val, found);
		if (found) {
			// DevNote: Replace / Ignore / Error
			return; // currently it's ignore semantic
		}
		m_data.push_back(val);
		u32 nodeIndex = makeNewNode_impl();

		if (isNull(parentIndex)) {
			m_meta.root = nodeIndex;
			return;
		}

		// add node
		m_nodes[nodeIndex].parent = parentIndex;
		Node_impl& parent = m_nodes[parentIndex];
		Direction bin = static_cast<Direction>(cmp(parent, val));
		parent.children[bin] = nodeIndex;
		// resolve insersion
		if (isNull(parent.child(other(bin)))) // if the other child is null
			insert_impl(nodeIndex); // which means that the height increased
		else { // as height of parent don't change, only updating bf of parent
			m_nodemeta[parentIndex].bf += binToBf(bin);
		}
	}

private:
	// just treat this as boolean / bin (binary) in general
	// This is both the direction for rotation (right or left rotation), and
	// the index / position of child in a node (left or right child)
	// Left maps to child[0], Right maps to child[1]
	// often refer to "bin" in this implementation
	enum Direction : u32 {
		Left = 0,
		Right = 1
	};

	struct Node_impl {
		Node_impl() = default;
		Node_impl(u32 in_data) : data(in_data), parent(null), children({ null, null }) {}
		u32 data, parent; // data have to not be null at all time
		std::array<u32, 2> children; // first one is left, second is right

		u32& child(Direction dir) { return children[dir]; }
		u32 child(Direction dir) const { return children[dir]; }

		/**
		 * There way node (parent + 2 children)
		 * Plus data ptr
		 */
	};
	// Balance Factors (SoA to save memory so Node_impl don't become 20 bytes)
	struct NodeMeta_impl {
		i8 bf;
	};

	inline static constexpr const u32 null = InvalidU32;

private:
	std::vector<T> m_data;
	std::vector<Node_impl> m_nodes; // nodes
	std::vector<NodeMeta_impl> m_nodemeta;

	CmpFunc m_cmp;

	struct Meta_impl {
		u32 root = null;
	} m_meta;

	/**
	 * @param index the index of the node that is MOVING UP,
	 * not the imbalance node it self
	 */
	void rotate_impl(Direction dir, u32 index) {
		u32 nodeIndex = index;
		Node_impl& node = m_nodes[nodeIndex];
		u32 parentIndex = node.parent;
		Node_impl& parent = m_nodes[parentIndex];

		i8 oldNodeBf = m_nodemeta[nodeIndex].bf;
		i8 oldParentBf = m_nodemeta[parentIndex].bf;

		if (dir == Left) {
			m_nodemeta[parentIndex].bf = oldParentBf - 1 - std::max(i8{ 0 }, oldNodeBf);
			m_nodemeta[nodeIndex].bf = oldNodeBf - 1 + std::min(i8{ 0 }, m_nodemeta[parentIndex].bf);
		} else {
			m_nodemeta[parentIndex].bf = oldParentBf + 1 - std::min(i8{ 0 }, oldNodeBf);
			m_nodemeta[nodeIndex].bf = oldNodeBf + 1 + std::max(i8{ 0 }, m_nodemeta[parentIndex].bf);
		}
		/**
		 * using example for right rotation `parent`:
		 * `m_nodemeta[parentIndex].bf = oldParentBf + 1 - std::min(i8{ 0 }, oldNodeBf);`
		 * this can be separated into 2 parts:
		 * - `oldParentBf + 1`
		 * - `std::min(i8{ 0 }, oldNodeBf)`
		 * 
		 * The first part is the normal canceling: originally there requires 2
		 * jumps to get to `child` from `parent`. now after connecting `child`
		 * and `parent` directly, there's only one required, therefore 1 is
		 * removed from the height of the right side of parent node (in this
		 * case), which results the increase of 1 on bf.
		 * 
		 * The second part is accounting for the height of the `child` node.
		 * Since before the tree was balanced, but `node` could be right heavy,
		 * and have the other child then `child` have one more height.
		 * Now `parent` adopted the `child` branch, but if the other child have
		 * one more then `child`, that means `parent` actually losted one more
		 * height then before, which result another 1 decreased on right, which
		 * result the total bf to increase by 1.
		 */


		// connect child to parent
		u32 childIndex = node.child(other(dir));
		if (!isNull(childIndex)) {
			Node_impl& child = m_nodes[childIndex];
			parent.child(dir) = childIndex;
			child.parent = parentIndex;
		} else {
			parent.child(dir) = null;
		}

		// connect node to grand
		u32 grandIndex = parent.parent;
		if (!isNull(grandIndex)) {
			Node_impl& grand = m_nodes[grandIndex];
			u32 grandChildIndex = binAtParent(parentIndex);
			grand.child(static_cast<Direction>(grandChildIndex)) = nodeIndex;
			node.parent = grandIndex;
		} else { // parent orginally is root - change note to root
			m_meta.root = nodeIndex;
			node.parent = null;
		}

		// connect parent to node
		// have to be at the end because `node.child(!dir)` and `parent.parent` is changed here
		node.child(other(dir)) = parentIndex;
		parent.parent = nodeIndex;
	}

	/**
	 * call on the node that is imbalance
	 */
	void balance_impl(u32 index) {
		u32 nodeIndex = index;
		Node_impl node = m_nodes[nodeIndex];
		i8 nodeBf = m_nodemeta[nodeIndex].bf;

		Direction direction = bfToBin(nodeBf);
		u32 childIndex = node.child(direction);
		Node_impl child = m_nodes[childIndex];
		i8 childBf = m_nodemeta[childIndex].bf;

		// target is the nodeIndex of the node that will be the root after the
		// rotations.
		if (different(nodeBf, childBf)) { // double rotation
			u32 target = child.child(other(direction));
			// opposite direction rotation
			rotate_impl(other(direction), target);
			// direction rotation
			rotate_impl(direction, target);

		} else { // single rotation
			u32 target = childIndex;
			// direction rotate for the target child of node
			rotate_impl(direction, target);
		}
	}

	/**
	 * return the closest node to the targeting node.
	 * It could be the node it self, or the parent of it (as if it exists)
	 */
	u32 search_impl(const T& val, bool& same) {
		same = false;
		if (m_nodes.empty()) return null;

		u32 curIndex = m_meta.root;
		while (true) {
			const Node_impl& cur = m_nodes[curIndex];
			u32 nxt;
			if (cmp(val, cur)) { // left
				nxt = cur.child(Direction::Left);
			} else if (cmp(cur, val)) { // right
				nxt = cur.child(Direction::Right);
			} else { // equal
				nxt = null;
				same = true;
			}

			if (isNull(nxt))
				return curIndex;
			else
				curIndex = nxt;
		}
	}
	/**
	 * Technically it's `resolveInsertion` / `rebalance`,
	 * but named `insert_impl` just for naming alignment.
	 * Should be ran after insertion, where the last element of m_nodes is
	 * the newly inserted node.
	 * 
	 * This function is responsible of updating the bf, and
	 * perform potensial rotations
	 * 
	 * The purpose of this separation is to isolate the value from the AVL logic
	 * 
	 * @param index the index of the node that just got inserted
	 */
	void insert_impl(u32 index) {
		u32 prevIndex = index;
		u32 curIndex = m_nodes[index].parent;

		while (true) {
			// update bf
			i8& curBf = m_nodemeta[curIndex].bf;
			curBf += binToBf(binAtParent(prevIndex));

			// if perfect balance
			// early return, because that means the newly inserted node had
			// filled up a slot in the tree
			if (curBf == 0) return;

			// if imbalance
			// trigger rebalance
			if (!isBalanced(curBf)) {
				balance_impl(curIndex); // rebalance
				return;
				// return immediately after rebalance, because rebalance will
				// reduce the increased height, pull the height back to what it
				// started with.
			}

			// update indices for next iteration
			if (curIndex == m_meta.root) return;
			prevIndex = curIndex;
			curIndex = m_nodes[curIndex].parent;
		}
	}




	// called immediately after pushing a new element in m_data
	u32 makeNewNode_impl() {
		m_nodes.push_back(Node_impl(m_data.size() - 1));
		m_nodemeta.push_back(NodeMeta_impl{ 0 });
		return m_nodes.size() - 1;
	}


	// helper functions

	static bool isNull(u32 index) { return index == null; }
	bool cmp(const Node_impl& node, const T& val) const { return m_cmp(m_data[node.data], val); }
	bool cmp(const T& val, const Node_impl& node) const { return m_cmp(val, m_data[node.data]); }
	bool isSame(const T& a, const T& b) const { return !m_cmp(a, b) && !m_cmp(b, a); }
	bool isSame(const Node_impl& node, const T& val) const { return isSame(m_data[node.data], val); }

	static bool isBalanced(i8 bf) { return std::abs(bf) <= 1; }
	static Direction bfToBin(i8 bf) { return bf < 0 ? Direction::Right : Direction::Left; }
	static i8 binToBf(Direction bin) { return bin == Direction::Right ? i8{ -1 } : i8{ 1 }; }
	static bool different(i8 bf1, i8 bf2) {
		return (bf1 * bf2) < 0;
	}
	Direction binAtParent(u32 nodeIndex) const {
		// (it's all trick - zero readability -- TX_Jerry)
		//return m_nodes[m_nodes[nodeIndex].parent].children[1] == nodeIndex ? Direction::Right : Direction::Left;
		return static_cast<Direction>(m_nodes[m_nodes[nodeIndex].parent].children[1] == nodeIndex);
	}
	static Direction other(Direction val) { return val == Direction::Right ? Direction::Left : Direction::Right; }
};


} // namespace tx