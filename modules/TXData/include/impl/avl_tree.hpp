// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <vector>
#include <array>
#include <utility>

#include <cassert>
#include <iostream>

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



	FindResult search(const T& val) {
		FindResult result;
		u32 nodeIndex = search_impl(val, result.found);
		if (result) result.value = &m_data[nodeIndex];
		return result;
	}

	// void insert(const T& val) {
	// 	bool found = false;
	// 	u32 parentIndex = search_impl(val, found);
	// 	if (found) {
	// 		// DevNote: Replace / Ignore / Error
	// 		return; // currently it's ignore semantic
	// 	}
	// 	m_data.push_back(val);
	// 	u32 nodeIndex = makeNewNode_impl();

	// 	if (isNull(parentIndex)) {
	// 		m_meta.root = nodeIndex;
	// 		return;
	// 	}

	// 	// add node
	// 	m_nodes[nodeIndex].parent = parentIndex;
	// 	Node_impl& parent = m_nodes[parentIndex];
	// 	Direction bin = static_cast<Direction>(cmp(parentIndex, val));
	// 	parent.children[bin] = nodeIndex;
	// 	// resolve insersion
	// 	if (isNull(parent.child(other(bin)))) // if the other child is null
	// 		propagate_impl(parentIndex, binAtParent(nodeIndex, parentIndex), 1); // which means that the height increased
	// 	else { // as height of parent don't change, only updating bf of parent
	// 		assert(std::abs(m_nodemeta[parentIndex].bf) != 0);
	// 		m_nodemeta[parentIndex].bf += binToBf(bin);
	// 	}
	// }

	void insert(const T& val) {
		bool found = false;
		u32 parentIndex = search_impl(val, found);
		if (found) {
			// DevNote: Replace / Ignore / Error
			return; // currently it's ignore semantic
		}
		std::cout << "parent is " << parentIndex << '\n';
		m_data.push_back(val);
		u32 nodeIndex = makeNewNode_impl();
		if (isNull(parentIndex)) {
			m_meta.root = nodeIndex;
			return;
		}
		// add node
		m_nodes[nodeIndex].parent = parentIndex;
		Node_impl& parent = m_nodes[parentIndex];
		Direction bin = static_cast<Direction>(cmp(parentIndex, val));
		parent.children[bin] = nodeIndex;
		// resolve insersion
		if (isNull(parent.child(other(bin)))) { // if the other child is null
			std::cout << "parent was leaf, height changed, propagation begin.\n";
			propagate_impl(parentIndex, binAtParent(nodeIndex, parentIndex), 1); // which means that the height increased
		} else { // as height of parent don't change, only updating bf of parent
			std::cout << "parent have child, no height change, only updating bf of parent.\n";
			assert(std::abs(m_nodemeta[parentIndex].bf) <= 2);
			assert(std::abs(m_nodemeta[parentIndex].bf) != 0);
			m_nodemeta[parentIndex].bf += binToBf(bin);
			// DevNote: Debug
			//std::cout << "******************* insert bf: " << (int)m_nodemeta[parentIndex].bf << '\n';
			assert(std::abs(m_nodemeta[parentIndex].bf) <= 2);
		}
	}


	void remove(const T& val) {
		bool found = false;
		u32 nodeIndex = search_impl(val, found);
		if (!found) return; // DevNote: maybe throw?
		delete_impl(nodeIndex);
	}





	// debug

	struct indent_t {
		u32 val;
	};
	static indent_t indent(u32 indent) {
		return indent_t{ indent };
	}
	friend std::ostream& operator<<(std::ostream& os, indent_t indent) {
		for (u32 i = 0; i < indent.val; i++) {
			os << ' ';
		}
		return os;
	}



	void printNode(u32 nodeIndex, std::ostream& os = std::cout, u32 in_indent = 0) {
		os << indent(in_indent) << "-------------------------- node [" << nodeIndex << "] status \n";
		if (m_nodes.size() <= nodeIndex) {
			os << indent(in_indent) << "Not yet created.\n";
			return;
		}
		Node_impl node = m_nodes[nodeIndex];
		os << indent(in_indent) << "Parent: " << node.parent << '\n'
		   << indent(in_indent) << "LeftChild: " << node.child(Direction::Left)
		   << "(Height: " << getHeight(node.child(Direction::Left)) << ")\n"
		   << indent(in_indent) << "RightChild: " << node.child(Direction::Right)
		   << "(Height: " << getHeight(node.child(Direction::Right)) << ")\n"
		   << indent(in_indent) << "bf: " << (int)m_nodemeta[nodeIndex].bf << '\n';
	}
	void printTree(u32 nodeIndex, std::ostream& os = std::cout, u32 in_indent = 0) {
		printNode(nodeIndex, os, in_indent);
		if (!isNull(m_nodes[nodeIndex].child(Direction::Left)))
			printNode(m_nodes[nodeIndex].child(Direction::Left), os, in_indent + 4);
		if (!isNull(m_nodes[nodeIndex].child(Direction::Right)))
			printNode(m_nodes[nodeIndex].child(Direction::Right), os, in_indent + 4);
	}
	void printTreeRecursive(u32 nodeIndex, std::ostream& os = std::cout, u32 in_indent = 0) {
		printNode(nodeIndex, os, in_indent);
		if (!isNull(m_nodes[nodeIndex].child(Direction::Left)))
			printTreeRecursive(m_nodes[nodeIndex].child(Direction::Left), os, in_indent + 4);
		if (!isNull(m_nodes[nodeIndex].child(Direction::Right)))
			printTreeRecursive(m_nodes[nodeIndex].child(Direction::Right), os, in_indent + 4);
	}
	void printAll(std::ostream& os = std::cout) {
		printTreeRecursive(m_meta.root, os);
	}

	u32 getHeight(u32 nodeIndex) {
		if (isNull(nodeIndex)) return 0;
		u32 leftHeight = getHeight(m_nodes[nodeIndex].child(Direction::Left));
		u32 rightHeight = getHeight(m_nodes[nodeIndex].child(Direction::Right));

		return std::max(leftHeight, rightHeight) + 1;
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
		Node_impl() : parent(null), children({ null, null }) {}
		u32 parent; // data have to not be null at all time
		std::array<u32, 2> children; // first one is left, second is right

		u32& child(Direction dir) { return children[dir]; }
		u32 child(Direction dir) const { return children[dir]; }

		/**
		 * Three way node (parent + 2 children)
		 */
	};
	// Balance Factors (SoA to save memory so Node_impl don't become 16 bytes)
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

		std::cout << "Rotation: Node: " << nodeIndex << '\n';
		std::cout << "Rotation: Parent: " << parentIndex << '\n';

		i8 oldNodeBf = m_nodemeta[nodeIndex].bf;
		i8 oldParentBf = m_nodemeta[parentIndex].bf;

		assert(std::abs(m_nodemeta[nodeIndex].bf) <= 3);
		assert(std::abs(m_nodemeta[parentIndex].bf) <= 3);

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
		std::cout << "Rotation: child: " << childIndex << '\n';
		if (!isNull(childIndex)) {
			Node_impl& child = m_nodes[childIndex];
			parent.child(dir) = childIndex;
			child.parent = parentIndex;
		} else {
			parent.child(dir) = null;
		}

		// connect node to grand
		u32 grandIndex = parent.parent;
		std::cout << "Rotation: grand: " << grandIndex << '\n';
		if (!isNull(grandIndex)) {
			Node_impl& grand = m_nodes[grandIndex];
			Direction grandChildIndex = binAtParent(parentIndex);
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
	 * @return the index of the node that replaces the position of param `index`
	 */
	u32 balance_impl(u32 index) {
		u32 nodeIndex = index;
		Node_impl node = m_nodes[nodeIndex];
		i8 nodeBf = m_nodemeta[nodeIndex].bf;

		Direction direction = bfToBin(nodeBf);
		u32 childIndex = node.child(direction);
		Node_impl child = m_nodes[childIndex];
		i8 childBf = m_nodemeta[childIndex].bf;

		assert(std::abs(nodeBf) <= 3);
		assert(std::abs(childBf) <= 2);

		// target is the nodeIndex of the node that will be the root after the
		// rotations.
		if (different(nodeBf, childBf)) { // double rotation
			std::cout << "Balance: Double Rotation\n";
			u32 target = child.child(other(direction));
			// opposite direction rotation
			rotate_impl(other(direction), target);
			// direction rotation
			rotate_impl(direction, target);

			assert(std::abs(m_nodemeta[target].bf) <= 2);

			return target;
		} else { // single rotation
			std::cout << "Balance: Single Rotation\n";
			u32 target = childIndex;
			// direction rotate for the target child of node
			rotate_impl(direction, target);

			assert(std::abs(m_nodemeta[target].bf) <= 2);

			return target;
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
			if (pmc(curIndex, val)) { // left
				nxt = cur.child(Direction::Left);
			} else if (cmp(curIndex, val)) { // right
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
	 * Backward propagation
	 * 
	 * This function is responsible of updating the bf, and
	 * perform potensial rotations
	 * 
	 * Note: don't require the object of nodeIndex still alive, but require the
	 * object of parentIndex still alive
	 * 
	 * @param parentIndex the node that is affected by the operation, the 
	 * parent of the node that just changed (inserted or deleted)
	 * @param nodePosition the bin of `node` in `parent`
	 * @param bfDiff the indicater of propagation type:
	 * 1 is insertion, -1 is deletion
	 */
	void propagate_impl(u32 parentIndex, Direction nodePosition, i8 bfDiff) {
		// cur is the parent
		u32 curIndex = parentIndex;
		Direction prevBin = nodePosition;

		while (true) {
			// update bf
			i8& curBf = m_nodemeta[curIndex].bf;
			assert(std::abs(curBf) <= 2);
			curBf += bfDiff * binToBf(prevBin);

			std::cout << "Propagation: accessing " << curIndex << '\n';

			// update indices for next iteration
			u32 prevIndex = curIndex;

			// early return for deletion:
			// when bf is -1 or 1, there is no height change, because the
			// deletion is already consumed. (it deletes one of cur's child,
			// who has 2 children or in another word, decreased one of cur's
			// child's height leaving a non-perfect (!= 0) bf)
			if (bfDiff == -1 && isBalanced(curBf) && curBf != 0) {
				return;
			};

			// early return for insertion:
			// when curBf is 0 it is perfect balance, and means the newly
			// inserted node had filled up a slot in the tree
			//if (bfDiff == 1 && curBf == 0) return;

			// if imbalance
			// trigger rebalance
			if (!isBalanced(curBf)) {
				std::cout << "Propagation: imbalance node: " << curIndex << "; Balancing...\n";

				u32 root = balance_impl(curIndex); // rebalance

				assert(std::abs(m_nodemeta[root].bf) <= 2);

				// early return for insertion:
				// return immediately after rebalance, because rebalance will
				// reduce the increased height, pull the height back to what it
				// started with.
				//
				// But for deletion, since rebalance will garantee to decrease
				// the height by 1, the propagation have to continue to the root
				if (bfDiff == 1)
					return;
				else {
					// early return for deletion (same as before)
					if (m_nodemeta[root].bf != 0) return;
					// update the prevIndex to adapt the rotated structure
					if (root == m_meta.root) return;
					prevIndex = root;
					curIndex = m_nodes[root].parent;
					prevBin = binAtParent(prevIndex, curIndex);
					continue;
					// the deletion propagation continues
				}
			}

			// update indices for next iteration
			if (curIndex == m_meta.root) return;
			curIndex = m_nodes[curIndex].parent;
			prevBin = binAtParent(prevIndex, curIndex);
		}
	}

	/**
	 * for deletion - find the node to swap up to become the new root
	 * should only be called for node that have children, therefore the impl
	 * assumes children exist
	 * 
	 * convension: minimum bigger node / maximum smaller node (low priority)
	 *                                   <- this one is only used when `node`
	 *                                      have no right child
	 * 
	 * logic: go one node in direction, then go inv direction until can go no
	 * more
	 * 
	 * @return the index of the closest node from target
	 */
	u32 findClosest_impl(u32 nodeIndex) const {
		const Node_impl& node = m_nodes[nodeIndex];
		if (isNull(node.child(Direction::Right))) {
			// fallback (when there's no right child)
			// return left directly, since left cannot have any child (or it
			// would be imbalance)
			return node.child(Direction::Left);
		} else {
			// normal search
			u32 curIndex = node.child(Direction::Right);
			u32 nxtIndex = m_nodes[curIndex].child(Direction::Left);
			while (!isNull(nxtIndex)) {
				curIndex = nxtIndex;
				nxtIndex = m_nodes[curIndex].child(Direction::Left);
			}
			return curIndex;
		}
	}

	/**
	 * AVL delete
	 * The pipeline:
	 * 1. if target node is leaf, just delete it and start the propagation
	 * 2. else: there is child to this node. find the node in it's child nodes
	 * to swap with (to replace the removing node)
	 * 3. connect all reference of target node to found swapping node
	 * 4. delete (in memory) the target node
	 * 5. resolve child of swaping node (connect to swaping node's parent)
	 * 
	 * There are 4 cases in deletion:
	 * 1. target is leaf - easy leaf delete
	 * 2. target is tree, and swapper have a parent that's not target
	 * 3. target is tree, and swapper is the left child of target
	 * 4. traget is tree, and swapper is the right child of target
	 */
	void delete_impl(u32 nodeIndex) {
		const Node_impl& node = m_nodes[nodeIndex]; // target
		std::cout << "*********************** Deletion: deleting node " << nodeIndex << '\n';

		// case 1: target is leaf
		if (isNull(node.children[0]) &&
		    isNull(node.children[1])) {
			// DevNote: Debug
			std::cout << "*********************** Deletion: Case 1\n";

			u32 parentIndex = node.parent;
			if (isNull(parentIndex)) {
				m_meta.root = null;
			} else {
				// unlink
				Direction nodeBin = binAtParent(nodeIndex, parentIndex);
				m_nodes[parentIndex].child(nodeBin) = null;

				propagate_impl(parentIndex, nodeBin, -1);
			}
			deleteNode_impl(nodeIndex);
			return;
		}

		u32 swapperIndex = findClosest_impl(nodeIndex);
		Node_impl& swapper = m_nodes[swapperIndex];
		u32 swapperParentIndex = swapper.parent;

		u32 nodeParentIndex = node.parent;

		if (swapperParentIndex != nodeIndex) {
			// case 2: target is tree, and swapper have a parent that's not target
			// In this case, swapper have at most one right child:
			// Since there is no left child (findClosest reached the end) of
			// swapper, to be balance the maximum number of height swapper can have
			// on right it 1.

			// DevNote: Debug
			std::cout << "*********************** Deletion: Case 2\n";

			Direction swapperBin = binAtParent(swapperIndex, swapperParentIndex);

			// resolve swapper's child - it will replace swapper's place
			u32 swapperChildIndex = swapper.child(Direction::Right);
			// if swapperChildIndex is null then swapper is leaf, no child,
			// swapperParent point to null. Otherwise swapperParent points to
			// swapperChild
			m_nodes[swapperParentIndex].child(swapperBin) = swapperChildIndex;
			if (!isNull(swapperChildIndex)) {
				m_nodes[swapperChildIndex].parent = swapperParentIndex;
			}

			moveNode(nodeIndex, swapperIndex);
			// invalidating Node_impl& swapper.
			// -- comment requested by Claude

			propagate_impl(swapperParentIndex, swapperBin, -1);
			deleteNode_impl(nodeIndex);
			return;
		}

		if (binAtParent(swapperIndex, nodeIndex) == Direction::Left) {
			// case 3: target is tree, and swapper is the left child of target
			// In this case, swapper have to be a leaf:
			// Since there is no right child in node, if swapper have child, it
			// will be an imbalance

			// DevNote: Debug
			std::cout << "*********************** Deletion: Case 3\n";

			if (isNull(nodeParentIndex)) { // node is root
				swapper.parent = null;
				m_meta.root = swapperIndex;
			} else {
				Direction nodeBin = binAtParent(nodeIndex, nodeParentIndex);

				swapper.parent = nodeParentIndex;
				m_nodes[nodeParentIndex].child(nodeBin) = swapperIndex;

				propagate_impl(nodeParentIndex, nodeBin, -1);
			}

			deleteNode_impl(nodeIndex);
		} else {
			// case 4: traget is tree, and swapper is the right child of target
			// In this case, swapper have at most one right child:
			// Since there is no left child in swapper, the max height of swapper is
			// 2.
			// And the other branch could have a maximum height of 3.

			// DevNote: Debug
			std::cout << "*********************** Deletion: Case 4\n";

			// update swapper (new root)'s bf
			// + 1 for the decrease on right child
			i8& bf = m_nodemeta[swapperIndex].bf;
			assert(std::abs(bf) <= 2);
			bf = m_nodemeta[nodeIndex].bf + 1;

			// connect potential left child of node to swapper
			u32 nodeChildIndex = node.child(Direction::Left);
			if (!isNull(nodeChildIndex)) {
				Node_impl& nodeChild = m_nodes[nodeChildIndex];
				nodeChild.parent = swapperIndex;
				swapper.child(Direction::Left) = nodeChildIndex;
			}

			// at this point: the individual tree is ready, but potential
			// imbalance exists - solve potential imbalance
			u32 rootIndex = swapperIndex;
			if (!isBalanced(bf)) {
				rootIndex = balance_impl(swapperIndex);
			}
			Node_impl& root = m_nodes[rootIndex];

			// replace swapper to node (connect to node's parent)
			if (isNull(nodeParentIndex)) { // node is root
				root.parent = null;
				m_meta.root = rootIndex;
			} else {
				Direction nodeBin = binAtParent(nodeIndex, nodeParentIndex);

				root.parent = nodeParentIndex;
				m_nodes[nodeParentIndex].child(nodeBin) = rootIndex;

				// Propagation is not certain here. Wether height will change
				// depends on what bf of the node is.
				// if original is left heavy (1), decrease right height will
				// balance it (become 0);
				// if original is right heavy (-1), decrease right height will
				// create imbalance, which then the balance_impl() will decrease
				// the height - since by definition balance_impl() decrease
				// height by 1.
				// If it was balanced, decrease right height will just make it
				// into 1 without height change, since the other child's branch
				// remains the same height, keeping the overall height the same
				if (m_nodemeta[nodeIndex].bf != 0)
					propagate_impl(nodeParentIndex, nodeBin, -1);
			}

			deleteNode_impl(nodeIndex);
		}
	}


	// memory oriented helpers

	// called immediately after pushing a new element in m_data
	u32 makeNewNode_impl() {
		m_nodes.push_back(Node_impl{});
		m_nodemeta.push_back(NodeMeta_impl{ 0 });
		return m_nodes.size() - 1;
	}

	/**
	 * uses swap-to-back trick
	 * this is just memory operations and handling the index mismatch caused
	 * by the swap
	 * The old references of nodeIndex is not accounted
	 */
	void deleteNode_impl(u32 nodeIndex) {
		u32 src = m_nodes.size() - 1;
		// perform actual deletion
		if (nodeIndex < src) { // not already at back
			// move but not swap to save potential move assignemnt overhead
			// since the moving to back data is already deprecated
			m_data[nodeIndex] = std::move(m_data.back());
			moveNode(src, nodeIndex);
		}
		m_data.pop_back();
		m_nodes.pop_back();
		m_nodemeta.pop_back();
	}


	// helper functions

	static bool isNull(u32 index) { return index == null; }
	bool cmp(u32 nodeIndex, const T& val) const { return m_cmp(m_data[nodeIndex], val); }
	bool pmc(u32 nodeIndex, const T& val) const { return m_cmp(val, m_data[nodeIndex]); } // reverse of cmp
	bool isSame(const T& a, const T& b) const { return !m_cmp(a, b) && !m_cmp(b, a); }

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
	Direction binAtParent(u32 nodeIndex, u32 parentIndex) const { // provided parentIndex to avoid access of nodeIndex
		return static_cast<Direction>(m_nodes[parentIndex].children[1] == nodeIndex);
	}
	static Direction other(Direction val) { return val == Direction::Right ? Direction::Left : Direction::Right; }
	void moveNode_meta(u32 src, u32 dest) {
		m_nodes[dest] = m_nodes[src];
		m_nodemeta[dest] = m_nodemeta[src];
	}
	void moveNode_ref(u32 src, u32 dest) {
		const Node_impl& node = m_nodes[src];

		u32 parentIndex = node.parent;
		u32 child0Index = node.children[0];
		u32 child1Index = node.children[1];
		if (!isNull(parentIndex))
			m_nodes[parentIndex].child(
			    binAtParent(src, parentIndex)) = dest;
		else
			m_meta.root = dest;
		if (!isNull(child0Index))
			m_nodes[child0Index].parent = dest;
		if (!isNull(child1Index))
			m_nodes[child1Index].parent = dest;
	}
	void moveNode(u32 src, u32 dest) {
		moveNode_ref(src, dest);
		moveNode_meta(src, dest);
	}
};
} // namespace tx

/**
 * TODO:
 * - separate insert core logic out from insert to insert_impl
 *   - T&& insert support
 * - merge support
 * - foreach support
 * - conflict action traits
 */