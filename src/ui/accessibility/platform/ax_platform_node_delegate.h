// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_DELEGATE_H_
#define UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_DELEGATE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <new>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_clipping_behavior.h"
#include "ui/accessibility/ax_coordinate_system.h"
#include "ui/accessibility/ax_enums.mojom-forward.h"
#include "ui/accessibility/ax_export.h"
#include "ui/accessibility/ax_node_position.h"
#include "ui/accessibility/ax_offscreen_result.h"
#include "ui/accessibility/ax_position.h"
#include "ui/accessibility/ax_text_attributes.h"
#include "ui/accessibility/ax_text_utils.h"
#include "ui/accessibility/ax_tree.h"
#include "ui/accessibility/ax_tree_id.h"
#include "ui/accessibility/platform/ax_unique_id.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/native_widget_types.h"

namespace gfx {

class Rect;

}  // namespace gfx

namespace ui {

struct AXActionData;
struct AXNodeData;
struct AXTreeData;
class AXPlatformNode;

using TextAttribute = std::pair<std::string, std::string>;
using TextAttributeList = std::vector<TextAttribute>;

// A TextAttributeMap is a map between the text offset in UTF-16 characters in
// the node hypertext and the TextAttributeList that starts at that location.
// An empty TextAttributeList signifies a return to the default node
// TextAttributeList.
using TextAttributeMap = std::map<int, TextAttributeList>;

// An object that wants to be accessible should derive from this class.
// AXPlatformNode subclasses use this interface to query all of the information
// about the object in order to implement native accessibility APIs.
//
// Note that AXPlatformNode has support for accessibility trees where some
// of the objects in the tree are not implemented using AXPlatformNode.
// For example, you may have a native window with platform-native widgets
// in it, but in that window you have custom controls that use AXPlatformNode
// to provide accessibility. That's why GetParent, ChildAtIndex, HitTestSync,
// and GetFocus all return a gfx::NativeViewAccessible - so you can return a
// native accessible if necessary, and AXPlatformNode::GetNativeViewAccessible
// otherwise.
class AX_EXPORT AXPlatformNodeDelegate {
 public:
  using AXPosition = ui::AXNodePosition::AXPositionInstance;
  using SerializedPosition = ui::AXNodePosition::SerializedPosition;
  using AXRange = ui::AXRange<AXPosition::element_type>;

  AXPlatformNodeDelegate(const AXPlatformNodeDelegate&) = delete;
  AXPlatformNodeDelegate& operator=(const AXPlatformNodeDelegate&) = delete;

  virtual ~AXPlatformNodeDelegate() = default;

  // Get the accessibility data that should be exposed for this node. This data
  // is readonly and comes directly from the accessibility tree's source, e.g.
  // Blink.
  //
  // Virtually all of the information could be obtained from this structure
  // (role, state, name, cursor position, etc.) However, please prefer using
  // specific accessor methods, such as `GetStringAttribute` or
  // `GetTableCellRowIndex`, instead of directly accessing this structure,
  // because any attributes that could automatically be computed in the browser
  // process would also be returned. The browser process would try to correct
  // missing or erroneous information too.
  virtual const AXNodeData& GetData() const = 0;

  // Get the accessibility tree data for this node.
  virtual const AXTreeData& GetTreeData() const = 0;

  // Accessing accessibility attributes:
  //
  // There are dozens of possible attributes for an accessibility node,
  // but only a few tend to apply to any one object, so we store them
  // in sparse arrays of <attribute id, attribute value> pairs, organized
  // by type (bool, int, float, string, int list).
  //
  // There are three accessors for each type of attribute: one that returns
  // true if the attribute is present and false if not, one that takes a
  // pointer argument and returns true if the attribute is present (if you
  // need to distinguish between the default value and a missing attribute),
  // and another that returns the default value for that type if the
  // attribute is not present. In addition, strings can be returned as
  // either std::string or std::u16string, for convenience.

  virtual ax::mojom::Role GetRole() const = 0;
  virtual bool HasBoolAttribute(ax::mojom::BoolAttribute attribute) const = 0;
  virtual bool GetBoolAttribute(ax::mojom::BoolAttribute attribute) const = 0;
  virtual bool GetBoolAttribute(ax::mojom::BoolAttribute attribute,
                                bool* value) const = 0;
  virtual bool HasFloatAttribute(ax::mojom::FloatAttribute attribute) const = 0;
  virtual float GetFloatAttribute(
      ax::mojom::FloatAttribute attribute) const = 0;
  virtual bool GetFloatAttribute(ax::mojom::FloatAttribute attribute,
                                 float* value) const = 0;
  virtual const std::vector<std::pair<ax::mojom::IntAttribute, int32_t>>&
  GetIntAttributes() const = 0;
  virtual bool HasIntAttribute(ax::mojom::IntAttribute attribute) const = 0;
  virtual int GetIntAttribute(ax::mojom::IntAttribute attribute) const = 0;
  virtual bool GetIntAttribute(ax::mojom::IntAttribute attribute,
                               int* value) const = 0;
  virtual const std::vector<std::pair<ax::mojom::StringAttribute, std::string>>&
  GetStringAttributes() const = 0;
  virtual bool HasStringAttribute(
      ax::mojom::StringAttribute attribute) const = 0;
  virtual const std::string& GetStringAttribute(
      ax::mojom::StringAttribute attribute) const = 0;
  virtual bool GetStringAttribute(ax::mojom::StringAttribute attribute,
                                  std::string* value) const = 0;
  virtual std::u16string GetString16Attribute(
      ax::mojom::StringAttribute attribute) const = 0;
  virtual bool GetString16Attribute(ax::mojom::StringAttribute attribute,
                                    std::u16string* value) const = 0;
  virtual const std::string& GetInheritedStringAttribute(
      ax::mojom::StringAttribute attribute) const = 0;
  virtual std::u16string GetInheritedString16Attribute(
      ax::mojom::StringAttribute attribute) const = 0;
  virtual const std::vector<
      std::pair<ax::mojom::IntListAttribute, std::vector<int32_t>>>&
  GetIntListAttributes() const = 0;
  virtual bool HasIntListAttribute(
      ax::mojom::IntListAttribute attribute) const = 0;
  virtual const std::vector<int32_t>& GetIntListAttribute(
      ax::mojom::IntListAttribute attribute) const = 0;
  virtual bool GetIntListAttribute(ax::mojom::IntListAttribute attribute,
                                   std::vector<int32_t>* value) const = 0;
  virtual bool HasStringListAttribute(
      ax::mojom::StringListAttribute attribute) const = 0;
  virtual const std::vector<std::string>& GetStringListAttribute(
      ax::mojom::StringListAttribute attribute) const = 0;
  virtual bool GetStringListAttribute(
      ax::mojom::StringListAttribute attribute,
      std::vector<std::string>* value) const = 0;
  virtual bool HasHtmlAttribute(const char* attribute) const = 0;
  virtual const base::StringPairs& GetHtmlAttributes() const = 0;
  virtual bool GetHtmlAttribute(const char* attribute,
                                std::string* value) const = 0;
  virtual bool GetHtmlAttribute(const char* attribute,
                                std::u16string* value) const = 0;
  virtual AXTextAttributes GetTextAttributes() const = 0;
  virtual bool HasState(ax::mojom::State state) const = 0;
  virtual ax::mojom::State GetState() const = 0;
  virtual bool HasAction(ax::mojom::Action action) const = 0;
  bool HasDefaultActionVerb() const;
  std::vector<ax::mojom::Action> GetSupportedActions() const;
  virtual bool HasTextStyle(ax::mojom::TextStyle text_style) const = 0;
  virtual ax::mojom::NameFrom GetNameFrom() const = 0;
  virtual ax::mojom::DescriptionFrom GetDescriptionFrom() const = 0;

  // Returns the text of this node and all descendant nodes; including text
  // found in embedded objects.
  //
  // Only text displayed on screen is included. Text from ARIA and HTML
  // attributes that is either not displayed on screen, or outside this node,
  // e.g. aria-label and HTML title, is not returned.
  virtual std::u16string GetTextContentUTF16() const = 0;

  // Returns the value of a control such as a text field, a slider, a <select>
  // element, a date picker or an ARIA combo box. In order to minimize
  // cross-process communication between the renderer and the browser, may
  // compute the value from the control's inner text in the case of a text
  // field.
  virtual std::u16string GetValueForControl() const = 0;

  // Get the unignored selection from the tree, meaning the selection whose
  // endpoints are on unignored nodes. (An ignored node means that the node
  // should not be exposed to platform APIs: See `IsIgnored`.)
  virtual const AXTree::Selection GetUnignoredSelection() const = 0;

  // Creates a text position rooted at this object if it's a leaf node, or a
  // tree position otherwise.
  virtual AXNodePosition::AXPositionInstance CreatePositionAt(
      int offset,
      ax::mojom::TextAffinity affinity =
          ax::mojom::TextAffinity::kDownstream) const = 0;

  // Creates a text position rooted at this object.
  virtual AXNodePosition::AXPositionInstance CreateTextPositionAt(
      int offset,
      ax::mojom::TextAffinity affinity =
          ax::mojom::TextAffinity::kDownstream) const = 0;

  // Get the accessibility node for the NSWindow the node is contained in. This
  // method is only meaningful on macOS.
  virtual gfx::NativeViewAccessible GetNSWindow() = 0;

  // Get the node for this delegate, which may be an AXPlatformNode or it may
  // be a native accessible object implemented by another class.
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() = 0;

  // Get the parent of the node, which may be an AXPlatformNode or it may
  // be a native accessible object implemented by another class.
  virtual gfx::NativeViewAccessible GetParent() const = 0;

  // Get the index in parent. Typically this is the AXNode's index_in_parent_.
  // This should return nullopt if the index in parent is unknown.
  virtual absl::optional<size_t> GetIndexInParent() = 0;

  // Get the number of children of this node.
  //
  // Note that for accessibility trees that have ignored nodes, this method
  // should return the number of unignored children. All ignored nodes are
  // recursively removed from the children count. (An ignored node means that
  // the node should not be exposed to platform APIs: See
  // `IsIgnored`.)
  virtual size_t GetChildCount() const = 0;

  // Get a child of a node given a 0-based index.
  //
  // Note that for accessibility trees that have ignored nodes, this method
  // returns only unignored children. All ignored nodes are recursively removed.
  // (An ignored node means that the node should not be exposed to platform
  // APIs: See `IsIgnored`.)
  virtual gfx::NativeViewAccessible ChildAtIndex(size_t index) = 0;

  // Returns true if it has a modal dialog.
  virtual bool HasModalDialog() const = 0;

  // Gets the first child of a node, or nullptr if no children exist.
  virtual gfx::NativeViewAccessible GetFirstChild() = 0;

  // Gets the last child of a node, or nullptr if no children exist.
  virtual gfx::NativeViewAccessible GetLastChild() = 0;

  // Gets the next sibling of a given node, or nullptr if no such node exists.
  virtual gfx::NativeViewAccessible GetNextSibling() = 0;

  // Gets the previous sibling of a given node, or nullptr if no such node
  // exists.
  virtual gfx::NativeViewAccessible GetPreviousSibling() = 0;

  // Returns true if an ancestor of this node (not including itself) is a
  // leaf node, meaning that this node is not actually exposed to any
  // platform's accessibility layer.
  virtual bool IsChildOfLeaf() const = 0;

  // Returns true if this node is either an atomic text field , or one of its
  // ancestors is. An atomic text field does not expose its internal
  // implementation to assistive software, appearing as a single leaf node in
  // the accessibility tree. It includes <input>, <textarea> and Views-based
  // text fields.
  virtual bool IsDescendantOfAtomicTextField() const = 0;

  // Returns true if this object is at the root of what most accessibility APIs
  // consider to be a document, such as the root of a webpage, an iframe, or a
  // PDF.
  virtual bool IsPlatformDocument() const = 0;

  // Returns true if this node is ignored and should be hidden from the
  // accessibility tree. Methods that are used to navigate the accessibility
  // tree, such as "ChildAtIndex", "GetParent", and "GetChildCount", among
  // others, also skip ignored nodes. This does not impact the node's
  // descendants.
  //
  // Only relevant for accessibility trees that support ignored nodes.)
  virtual bool IsIgnored() const = 0;

  // Returns true if this is a leaf node, meaning all its
  // children should not be exposed to any platform's native accessibility
  // layer.
  virtual bool IsLeaf() const = 0;

  // Returns true if this node is invisible or ignored. (Only relevant for
  // accessibility trees that support ignored nodes.)
  virtual bool IsInvisibleOrIgnored() const = 0;

  // Returns true if this node is focused.
  virtual bool IsFocused() const = 0;

  // Returns true if this is a top-level browser window that doesn't have a
  // parent accessible node, or its parent is the application accessible node on
  // platforms that have one.
  virtual bool IsToplevelBrowserWindow() = 0;

  // If this object is exposed to the platform's accessibility layer, returns
  // this object. Otherwise, returns the platform leaf or lowest unignored
  // ancestor under which this object is found.
  //
  // (An ignored node means that the node should not be exposed to platform
  // APIs: See `IsIgnored`.)
  virtual gfx::NativeViewAccessible GetLowestPlatformAncestor() const = 0;

  // If this node is within an editable region, returns the node that is at the
  // root of that editable region, otherwise returns nullptr. In accessibility,
  // an editable region is synonymous to a node with the kTextField role, or a
  // contenteditable without the role, (see `AXNodeData::IsTextField()`).
  virtual gfx::NativeViewAccessible GetTextFieldAncestor() const = 0;

  // If this node is within a container (or widget) that supports either single
  // or multiple selection, returns the node that represents the container.
  virtual gfx::NativeViewAccessible GetSelectionContainer() const = 0;

  // If within a table, returns the node representing the table.
  virtual gfx::NativeViewAccessible GetTableAncestor() const = 0;

  class ChildIterator {
   public:
    virtual ~ChildIterator() = default;
    bool operator==(const ChildIterator& rhs) const {
      return GetIndexInParent() == rhs.GetIndexInParent();
    }
    bool operator!=(const ChildIterator& rhs) const {
      return GetIndexInParent() != rhs.GetIndexInParent();
    }
    virtual ChildIterator& operator++() = 0;
    virtual ChildIterator& operator++(int) = 0;
    virtual ChildIterator& operator--() = 0;
    virtual ChildIterator& operator--(int) = 0;
    virtual gfx::NativeViewAccessible GetNativeViewAccessible() const = 0;
    virtual absl::optional<size_t> GetIndexInParent() const = 0;
    virtual AXPlatformNodeDelegate& operator*() const = 0;
    virtual AXPlatformNodeDelegate* operator->() const = 0;
  };
  virtual std::unique_ptr<AXPlatformNodeDelegate::ChildIterator>
  ChildrenBegin() = 0;
  virtual std::unique_ptr<AXPlatformNodeDelegate::ChildIterator>
  ChildrenEnd() = 0;

  // Returns the accessible name for the node.
  virtual const std::string& GetName() const = 0;

  // Returns the accessible description for the node.
  // An accessible description gives more information about the node in
  // contrast to the accessible name which is a shorter label for the node.
  virtual const std::string& GetDescription() const = 0;

  // Returns the text of this node and represent the text of descendant nodes
  // with a special character in place of every embedded object. This represents
  // the concept of text in ATK and IA2 APIs.
  virtual std::u16string GetHypertext() const = 0;
  // Temporary accessor method until hypertext is fully migrated to `AXNode`
  // from `AXPlatformNodeBase`.
  // TODO(nektar): Remove this once selection handling is fully migrated to
  // `AXNode`.
  virtual const std::map<int, int>& GetHypertextOffsetToHyperlinkChildIndex()
      const = 0;

  // Set the selection in the hypertext of this node. Depending on the
  // implementation, this may mean the new selection will span multiple nodes.
  virtual bool SetHypertextSelection(int start_offset, int end_offset) = 0;

  // Compute the text attributes map for the node associated with this
  // delegate, given a set of default text attributes that apply to the entire
  // node. A text attribute map associates a list of text attributes with a
  // given hypertext offset in this node.
  virtual TextAttributeMap ComputeTextAttributeMap(
      const TextAttributeList& default_attributes) const = 0;

  // Get the inherited font family name for text attributes. We need this
  // because inheritance works differently between the different delegate
  // implementations.
  virtual std::string GetInheritedFontFamilyName() const = 0;

  // Return the bounds of this node in the coordinate system indicated. If the
  // clipping behavior is set to clipped, clipping is applied. If an offscreen
  // result address is provided, it will be populated depending on whether the
  // returned bounding box is onscreen or offscreen.
  virtual gfx::Rect GetBoundsRect(
      const AXCoordinateSystem coordinate_system,
      const AXClippingBehavior clipping_behavior,
      AXOffscreenResult* offscreen_result = nullptr) const = 0;

  // Derivative utils for AXPlatformNodeDelegate::GetBoundsRect
  gfx::Rect GetClippedScreenBoundsRect(
      AXOffscreenResult* offscreen_result = nullptr) const;
  gfx::Rect GetUnclippedScreenBoundsRect(
      AXOffscreenResult* offscreen_result = nullptr) const;
  gfx::Rect GetClippedRootFrameBoundsRect(
      ui::AXOffscreenResult* offscreen_result = nullptr) const;
  gfx::Rect GetUnclippedRootFrameBoundsRect(
      ui::AXOffscreenResult* offscreen_result = nullptr) const;
  gfx::Rect GetClippedFrameBoundsRect(
      ui::AXOffscreenResult* offscreen_result = nullptr) const;
  gfx::Rect GetUnclippedFrameBoundsRect(
      ui::AXOffscreenResult* offscreen_result = nullptr) const;

  // Return the bounds of the text range given by text offsets relative to
  // GetHypertext in the coordinate system indicated. If the clipping behavior
  // is set to clipped, clipping is applied. If an offscreen result address is
  // provided, it will be populated depending on whether the returned bounding
  // box is onscreen or offscreen.
  virtual gfx::Rect GetHypertextRangeBoundsRect(
      const int start_offset,
      const int end_offset,
      const AXCoordinateSystem coordinate_system,
      const AXClippingBehavior clipping_behavior,
      AXOffscreenResult* offscreen_result = nullptr) const = 0;

  // Return the bounds of the text range given by text offsets relative to
  // GetInnerText in the coordinate system indicated. If the clipping behavior
  // is set to clipped, clipping is applied. If an offscreen result address is
  // provided, it will be populated depending on whether the returned bounding
  // box is onscreen or offscreen.
  virtual gfx::Rect GetInnerTextRangeBoundsRect(
      const int start_offset,
      const int end_offset,
      const AXCoordinateSystem coordinate_system,
      const AXClippingBehavior clipping_behavior,
      AXOffscreenResult* offscreen_result = nullptr) const = 0;

  // Do a *synchronous* hit test of the given location in global screen physical
  // pixel coordinates, and the node within this node's subtree (inclusive)
  // that's hit, if any.
  //
  // If the result is anything other than this object or NULL, it will be
  // hit tested again recursively - that allows hit testing to work across
  // implementation classes. It's okay to take advantage of this and return
  // only an immediate child and not the deepest descendant.
  //
  // This function is mainly used by accessibility debugging software.
  // Platforms with touch accessibility use a different asynchronous interface.
  virtual gfx::NativeViewAccessible HitTestSync(
      int screen_physical_pixel_x,
      int screen_physical_pixel_y) const = 0;

  // Return the node within this node's subtree (inclusive) that currently has
  // focus, or return nullptr if this subtree is not connected to the top
  // document through its ancestry chain.
  virtual gfx::NativeViewAccessible GetFocus() const = 0;

  // Get whether this node is offscreen.
  virtual bool IsOffscreen() const = 0;

  // Get whether this node is a minimized window.
  virtual bool IsMinimized() const = 0;

  // See AXNode::IsText().
  virtual bool IsText() const = 0;

  // Get whether this node is in web content.
  virtual bool IsWebContent() const = 0;

  // Get whether this node can be marked as read-only.
  virtual bool IsReadOnlySupported() const = 0;

  // Get whether this node is marked as read-only or is disabled.
  virtual bool IsReadOnlyOrDisabled() const = 0;

  // Returns true if the caret or selection is visible on this object.
  virtual bool HasVisibleCaretOrSelection() const = 0;

  // Get another node from this same tree.
  virtual AXPlatformNode* GetFromNodeID(int32_t id) = 0;

  // Get a node from a different tree using a tree ID and node ID.
  // Note that this is only guaranteed to work if the other tree is of the
  // same type, i.e. it won't work between web and views or vice-versa.
  virtual AXPlatformNode* GetFromTreeIDAndNodeID(const ui::AXTreeID& ax_tree_id,
                                                 int32_t id) = 0;

  // Given a node ID attribute (one where IsNodeIdIntAttribute is true), return
  // a target nodes for which this delegate's node has that relationship
  // attribute or NULL if there is no such relationship.
  virtual AXPlatformNode* GetTargetNodeForRelation(
      ax::mojom::IntAttribute attr) = 0;

  // Given a node ID attribute (one where IsNodeIdIntListAttribute is true),
  // return a vector of all target nodes for which this delegate's node has that
  // relationship attribute.
  virtual std::vector<AXPlatformNode*> GetTargetNodesForRelation(
      ax::mojom::IntListAttribute attr) = 0;

  // Given a node ID attribute (one where IsNodeIdIntAttribute is true), return
  // a set of all source nodes that have that relationship attribute between
  // them and this delegate's node.
  virtual std::set<AXPlatformNode*> GetReverseRelations(
      ax::mojom::IntAttribute attr) = 0;

  // Given a node ID list attribute (one where IsNodeIdIntListAttribute is
  // true), return a set of all source nodes that have that relationship
  // attribute between them and this delegate's node.
  virtual std::set<AXPlatformNode*> GetReverseRelations(
      ax::mojom::IntListAttribute attr) = 0;

  // Return the string representation of the unique ID assigned by the author,
  // otherwise return an empty string. The author ID must be persistent across
  // any instance of the application, regardless of locale. The author ID should
  // be unique among sibling accessibility nodes and is best if unique across
  // the application, however, not meeting this requirement is non-fatal.
  virtual std::u16string GetAuthorUniqueId() const = 0;

  virtual const AXUniqueId& GetUniqueId() const = 0;

  // Return a vector of all the descendants of this delegate's node. This method
  // is only meaningful for Windows UIA.
  virtual const std::vector<gfx::NativeViewAccessible>
  GetUIADirectChildrenInRange(ui::AXPlatformNodeDelegate* start,
                              ui::AXPlatformNodeDelegate* end) = 0;

  // Return a string representing the language code.
  //
  // For web content, this will consider the language declared in the DOM, and
  // may eventually attempt to automatically detect the language from the text.
  //
  // This language code will be BCP 47.
  //
  // Returns empty string if no appropriate language was found or if this node
  // uses the default interface language.
  virtual std::string GetLanguage() const = 0;

  //
  // Tables. All of these should be called on a node that's a table-like
  // role, otherwise they return nullopt.
  //
  virtual bool IsTable() const = 0;
  virtual absl::optional<int> GetTableColCount() const = 0;
  virtual absl::optional<int> GetTableRowCount() const = 0;
  virtual absl::optional<int> GetTableAriaColCount() const = 0;
  virtual absl::optional<int> GetTableAriaRowCount() const = 0;
  virtual absl::optional<int> GetTableCellCount() const = 0;
  virtual absl::optional<bool> GetTableHasColumnOrRowHeaderNode() const = 0;
  virtual std::vector<int32_t> GetColHeaderNodeIds() const = 0;
  virtual std::vector<int32_t> GetColHeaderNodeIds(int col_index) const = 0;
  virtual std::vector<int32_t> GetRowHeaderNodeIds() const = 0;
  virtual std::vector<int32_t> GetRowHeaderNodeIds(int row_index) const = 0;
  virtual AXPlatformNode* GetTableCaption() const = 0;

  // Table row-like nodes.
  virtual bool IsTableRow() const = 0;
  virtual absl::optional<int> GetTableRowRowIndex() const = 0;

  // Table cell-like nodes.
  virtual bool IsTableCellOrHeader() const = 0;
  virtual absl::optional<int> GetTableCellIndex() const = 0;
  virtual absl::optional<int> GetTableCellColIndex() const = 0;
  virtual absl::optional<int> GetTableCellRowIndex() const = 0;
  virtual absl::optional<int> GetTableCellColSpan() const = 0;
  virtual absl::optional<int> GetTableCellRowSpan() const = 0;
  virtual absl::optional<int> GetTableCellAriaColIndex() const = 0;
  virtual absl::optional<int> GetTableCellAriaRowIndex() const = 0;
  virtual absl::optional<int32_t> GetCellId(int row_index,
                                            int col_index) const = 0;
  virtual absl::optional<int32_t> CellIndexToId(int cell_index) const = 0;

  // Returns true if this node is a cell or a row/column header in an ARIA grid
  // or treegrid.
  virtual bool IsCellOrHeaderOfAriaGrid() const = 0;

  // True if this is a web area, and its grandparent is a presentational iframe.
  virtual bool IsWebAreaForPresentationalIframe() const = 0;

  // Ordered-set-like and item-like nodes.
  virtual bool IsOrderedSetItem() const = 0;
  virtual bool IsOrderedSet() const = 0;
  virtual absl::optional<int> GetPosInSet() const = 0;
  virtual absl::optional<int> GetSetSize() const = 0;

  // Computed colors, taking blending into account.
  virtual SkColor GetColor() const = 0;
  virtual SkColor GetBackgroundColor() const = 0;

  //
  // Events.
  //

  // Return the platform-native GUI object that should be used as a target
  // for accessibility events.
  virtual gfx::AcceleratedWidget GetTargetForNativeAccessibilityEvent() = 0;

  //
  // Actions.
  //

  // Perform an accessibility action, switching on the ax::mojom::Action
  // provided in |data|.
  virtual bool AccessibilityPerformAction(const AXActionData& data) = 0;

  //
  // Localized strings.
  //

  virtual std::u16string GetLocalizedRoleDescriptionForUnlabeledImage()
      const = 0;
  virtual std::u16string GetLocalizedStringForImageAnnotationStatus(
      ax::mojom::ImageAnnotationStatus status) const = 0;
  virtual std::u16string GetLocalizedStringForLandmarkType() const = 0;
  virtual std::u16string GetLocalizedStringForRoleDescription() const = 0;
  virtual std::u16string GetStyleNameAttributeAsLocalizedString() const = 0;

  //
  // Testing.
  //

  // Accessibility objects can have the "hot tracked" state set when
  // the mouse is hovering over them, but this makes tests flaky because
  // the test behaves differently when the mouse happens to be over an
  // element. The default value should be false if not in testing mode.
  virtual bool ShouldIgnoreHoveredStateForTesting() = 0;

  // Creates a string representation of this delegate's data.
  std::string ToString() const { return GetData().ToString(); }

  // Returns a string representation of the subtree of delegates rooted at this
  // delegate.
  std::string SubtreeToString() { return SubtreeToStringHelper(0u); }

  friend std::ostream& operator<<(std::ostream& stream,
                                  const AXPlatformNodeDelegate& delegate) {
    return stream << delegate.ToString();
  }

 protected:
  AXPlatformNodeDelegate() = default;

  virtual std::string SubtreeToStringHelper(size_t level) = 0;
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_NODE_DELEGATE_H_
