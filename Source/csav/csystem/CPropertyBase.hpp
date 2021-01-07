#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <set>
#include <array>
#include <exception>

#include <utils.hpp>
#include <cpinternals/cpnames.hpp>
#include <csav/serializers.hpp>
#include <csav/csystem/CStringPool.hpp>
#include <csav/csystem/CSystemSerCtx.hpp>

#ifndef DISABLE_CP_IMGUI_WIDGETS
#include <widgets/list_widget.hpp>
#include <imgui_extras/cpp_imgui.hpp>
#include <imgui_extras/imgui_stdlib.h>
#endif

enum class EPropertyKind
{
  None,
  Unknown,
  Bool,
  Integer,
  Float,
  Double,
  Combo,
  Array,
  DynArray,
  Handle,
  Object,
  TweakDBID,
  CName,
  NodeRef
};


static inline std::string_view
property_kind_to_display_name(EPropertyKind prop_kind)
{
  switch (prop_kind)
  {
    case EPropertyKind::None: return "None";
    case EPropertyKind::Unknown: return "Unknown";
    case EPropertyKind::Bool: return "Bool";
    case EPropertyKind::Integer: return "Integer";
    case EPropertyKind::Float: return "Float";
    case EPropertyKind::Double: return "Double";
    case EPropertyKind::Combo: return "Combo";
    case EPropertyKind::Array: return "Array";
    case EPropertyKind::DynArray: return "DynArray";
    case EPropertyKind::Handle: return "Handle";
    case EPropertyKind::Object: return "Object";
  }
  return "Invalid";
}


enum class EPropertyEvent
{
  data_modified,
};

struct CPropertyListener
{
  virtual ~CPropertyListener() = default;
  virtual void on_cproperty_event(const std::shared_ptr<const CProperty>& prop, EPropertyEvent evt) = 0;
};


class CProperty
  : public std::enable_shared_from_this<const CProperty>
{
  EPropertyKind m_property_kind;
  bool m_is_skippable_in_ser = true;

protected:
  explicit CProperty(EPropertyKind kind)
    : m_property_kind(kind) {}

  virtual ~CProperty() = default;

public:
  EPropertyKind kind() const { return m_property_kind; }

  virtual CSysName ctypename() const = 0;

  bool is_skippable_in_serialization() const { return m_is_skippable_in_ser; }

  // serialization

protected:
  // when called, a data_modified event is sent automatically by base class' serialize_in(..)
  virtual bool serialize_in_impl(std::istream& is, CSystemSerCtx& serctx) = 0;

public:
  bool serialize_in(std::istream& is, CSystemSerCtx& serctx)
  {
    bool ok = serialize_in_impl(is, serctx);
    post_cproperty_event(EPropertyEvent::data_modified);
    return ok;
  }

  virtual bool serialize_out(std::ostream& os, CSystemSerCtx& serctx) const = 0;

  // gui (define DISABLE_CP_IMGUI_WIDGETS to disable implementations)

protected:
  // when returns true, a data_modified event is sent automatically by base class' imgui_widget(..)
  [[nodiscard]] virtual bool imgui_widget_impl(const char* label, bool editable)
  {
#ifndef DISABLE_CP_IMGUI_WIDGETS
    ImGui::Text("widget not implemented");
    return false;
#endif
  }

public:
  [[nodiscard]] bool imgui_widget(const char* label, bool editable)
  {
    bool modified = imgui_widget_impl(label, editable);
    if (modified)
      post_cproperty_event(EPropertyEvent::data_modified);
    return modified;
  }

  void imgui_widget(const char* label) const
  {
    std::ignore = const_cast<CProperty*>(this)->imgui_widget(label, false);
  }

  // events

protected:
  std::set<CPropertyListener*> m_listeners;

  void post_cproperty_event(EPropertyEvent evt) const
  {
    std::set<CPropertyListener*> listeners = m_listeners;
    for (auto& l : listeners) {
      l->on_cproperty_event(shared_from_this(), evt);
    }
    if (evt == EPropertyEvent::data_modified)
      const_cast<CProperty*>(this)->m_is_skippable_in_ser = false;
  }

public:
  // provided as const for ease of use

  void add_listener(CPropertyListener* listener) const
  {
    auto& listeners = const_cast<CProperty*>(this)->m_listeners;
    listeners.insert(listener);
  }

  void remove_listener(CPropertyListener* listener) const
  {
    auto& listeners = const_cast<CProperty*>(this)->m_listeners;
    listeners.erase(listener);
  }
};


//------------------------------------------------------------------------------
// DEFAULT
//------------------------------------------------------------------------------

class CUnknownProperty
  : public CProperty
{
protected:
  CSysName m_ctypename;
  std::vector<char> m_data;

public:
  explicit CUnknownProperty(CSysName ctypename)
    : CProperty(EPropertyKind::Unknown)
    , m_ctypename(ctypename)
  {
  }

  ~CUnknownProperty() override = default;

public:
  const std::vector<char>& raw_data() const { return m_data; };

  // overrides

  CSysName ctypename() const override { return m_ctypename; };

  bool serialize_in_impl(std::istream& is, CSystemSerCtx& serctx) override
  {
    std::streampos beg = is.tellg();
    is.seekg(0, std::ios_base::end);
    const size_t size = (size_t)(is.tellg() - beg);
    is.seekg(beg);
    m_data.resize(size);
    is.read(m_data.data(), size);
    //std::istream_iterator<char> it(is), end;
    //m_data.assign(it, end);
    return is.good();
  }

  virtual bool serialize_out(std::ostream& os, CSystemSerCtx& serctx) const
  {
    os.write(m_data.data(), m_data.size());
    return true;
  }

#ifndef DISABLE_CP_IMGUI_WIDGETS

  [[nodiscard]] bool imgui_widget_impl(const char* label, bool editable) override
  {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
      return false;

    bool modified = false;

    ImGui::BeginChild(label, ImVec2(0,0), true, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("refactoring");
    ImGui::Text("ctypename: %s", ctypename().str().c_str());
    ImGui::Text("data size: %08X", m_data.size());
    if (m_data.size() > 50)
      ImGui::Text("data: %s...", bytes_to_hex(m_data.data(), 50).c_str());
    else
      ImGui::Text("data: %s", bytes_to_hex(m_data.data(), m_data.size()).c_str());

    ImGui::EndChild();

    return modified;
  }

#endif
};

