//===- DIASupport.h - Common header includes for DIA ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Common defines and header includes for all LLVMDebugInfoPDBDIA.  The
// definitions here configure the necessary #defines and include system headers
// in the proper order for using DIA.
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_PDB_DIA_DIASUPPORT_H
#define LLVM_DEBUGINFO_PDB_DIA_DIASUPPORT_H

// Require at least Vista
#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define WINVER _WIN32_WINNT_VISTA
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cassert>
#include <cstddef>
#include <cwchar>
#include <utility>
#include <windows.h>
#include <oleauto.h>

// DIA headers must come after windows headers.
#include <cvconst.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
#include <dia2.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <diacreate.h>

template <typename T> class CComPtr {
public:
  CComPtr() noexcept = default;
  CComPtr(std::nullptr_t) noexcept {}
  CComPtr(T *Raw) noexcept : Ptr(Raw) { internalAddRef(); }
  CComPtr(const CComPtr &Other) noexcept : Ptr(Other.Ptr) { internalAddRef(); }
  CComPtr(CComPtr &&Other) noexcept : Ptr(Other.Ptr) { Other.Ptr = nullptr; }

  ~CComPtr() { internalRelease(); }

  CComPtr &operator=(T *Raw) noexcept {
    if (Ptr != Raw) {
      internalRelease();
      Ptr = Raw;
      internalAddRef();
    }
    return *this;
  }

  CComPtr &operator=(const CComPtr &Other) noexcept {
    if (this != &Other) {
      internalRelease();
      Ptr = Other.Ptr;
      internalAddRef();
    }
    return *this;
  }

  CComPtr &operator=(CComPtr &&Other) noexcept {
    if (this != &Other) {
      internalRelease();
      Ptr = Other.Ptr;
      Other.Ptr = nullptr;
    }
    return *this;
  }

  T *operator->() const noexcept { return Ptr; }
  T &operator*() const noexcept { return *Ptr; }
  operator T *() const noexcept { return Ptr; }
  explicit operator bool() const noexcept { return Ptr != nullptr; }

  T **operator&() noexcept {
    internalRelease();
    return &Ptr;
  }

  T *get() const noexcept { return Ptr; }

  void Attach(T *Raw) noexcept {
    internalRelease();
    Ptr = Raw;
  }

  T *Detach() noexcept {
    T *Result = Ptr;
    Ptr = nullptr;
    return Result;
  }

  void Release() noexcept { internalRelease(); }

  HRESULT CopyTo(T **Out) const noexcept {
    if (!Out)
      return E_POINTER;
    *Out = Ptr;
    internalAddRef(*Out);
    return S_OK;
  }

private:
  static void internalAddRef(T *Value) noexcept {
    if (Value)
      Value->AddRef();
  }

  void internalAddRef() const noexcept { internalAddRef(Ptr); }

  void internalRelease() noexcept {
    if (Ptr) {
      T *Value = Ptr;
      Ptr = nullptr;
      Value->Release();
    }
  }

  T *Ptr = nullptr;
};

class CComBSTR {
public:
  BSTR m_str = nullptr;

  CComBSTR() noexcept = default;
  explicit CComBSTR(const wchar_t *Value) : m_str(copyBSTR(Value)) {}
  CComBSTR(const CComBSTR &Other) : m_str(copyBSTR(Other.m_str)) {}
  CComBSTR(CComBSTR &&Other) noexcept : m_str(Other.m_str) {
    Other.m_str = nullptr;
  }

  ~CComBSTR() { ::SysFreeString(m_str); }

  CComBSTR &operator=(const wchar_t *Value) {
    assign(Value);
    return *this;
  }

  CComBSTR &operator=(const CComBSTR &Other) {
    if (this != &Other)
      assign(Other.m_str);
    return *this;
  }

  CComBSTR &operator=(CComBSTR &&Other) noexcept {
    if (this != &Other) {
      ::SysFreeString(m_str);
      m_str = Other.m_str;
      Other.m_str = nullptr;
    }
    return *this;
  }

  BSTR *operator&() noexcept {
    ::SysFreeString(m_str);
    m_str = nullptr;
    return &m_str;
  }

  operator BSTR() const noexcept { return m_str; }

  UINT ByteLength() const noexcept { return ::SysStringByteLen(m_str); }

  bool operator==(const wchar_t *Value) const noexcept {
    if (!m_str || !Value)
      return m_str == Value;
    return std::wcscmp(m_str, Value) == 0;
  }

private:
  static BSTR copyBSTR(const wchar_t *Value) {
    return Value ? ::SysAllocString(Value) : nullptr;
  }

  void assign(const wchar_t *Value) {
    BSTR NewValue = copyBSTR(Value);
    ::SysFreeString(m_str);
    m_str = NewValue;
  }
};

#endif // LLVM_DEBUGINFO_PDB_DIA_DIASUPPORT_H