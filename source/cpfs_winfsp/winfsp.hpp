#pragma once
#define WIN32_NO_STATUS
#define NOMINMAX
#include <winfsp/winfsp.h>
#include <sddl.h>

#include <cpinternals/common.hpp>
#include <cpinternals/common/windowz.hpp>

struct security_desc
{
  security_desc() = default;

  ~security_desc()
  {
    clear();
  }

  security_desc(const std::string& sddl)
  {
    set_sddl(sddl);
  }

  void clear()
  {
    if (m_psecdesc)
    {
      LocalFree(m_psecdesc);
      m_psecdesc = nullptr;
      m_secdesc_size = 0;
    }
  }

  // root: "O:BAG:BAD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FA;;;WD)";
  bool set_sddl(const std::string& sddl)
  {
    clear();

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(sddl.c_str(), SDDL_REVISION_1, &m_psecdesc, &m_secdesc_size))
    {
      SPDLOG_ERROR("security_desc::set_sddl: ", cp::windowz::get_last_error());
      return false;
    }

    return true;
  }

  PSECURITY_DESCRIPTOR get() const
  {
    return m_psecdesc;
  }

  ULONG size() const
  {
    return m_secdesc_size;
  }

private:

  PSECURITY_DESCRIPTOR m_psecdesc = nullptr;
  ULONG m_secdesc_size = 0;
};

inline constexpr uint64_t filetime_to_fsp_time(const FILETIME& ft)
{
  return ULARGE_INTEGER{ft.dwLowDateTime, ft.dwHighDateTime}.QuadPart;
}

inline constexpr LARGE_INTEGER fsp_time_to_largeinteger(uint64_t fspt)
{
  LARGE_INTEGER tmp{};
  tmp.QuadPart = fspt;
  return tmp;
}

bool set_file_times(HANDLE file_handle, cp::file_time creation, cp::file_time last_access, cp::file_time last_write, cp::file_time change);

