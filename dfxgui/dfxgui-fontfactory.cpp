 /*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2020  Sophia Poirier and Tom Murphy VII

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org/
------------------------------------------------------------------------*/

#include "dfxgui-fontfactory.h"

#include <memory>
#include <mutex>
#include <string>
#include <optional>
#include <vector>

#include "dfxdefines.h"
#include "dfxgui-base.h"
#include "dfxguimisc.h"

#include "vstgui.h"

#if TARGET_OS_WIN32
#include <windows.h>
// Some platform-specific vstgui peeking in order to load custom fonts.
// (XXX: GetInstance can probably now come from DfxGuiEditor, which would
// be a little cleaner?)
#include "lib/platform/win32/win32support.h"
#endif

// XXX might be unnecessary; moved some code from dfxguiedtior
#ifdef TARGET_API_AUDIOUNIT
#include "dfx-au-utilities.h"
#endif

#ifdef TARGET_API_VST
#include "dfxplugin.h"
#endif

#include "dfxtrace.h"

#if TARGET_OS_WIN32
// Must use the same flags to register and unregister.
// Would be nice if FR_PRIVATE worked (then other apps don't see the
// font, but it seems that VSTGUI won't find it either!)
#define FONT_RESOURCE_FLAGS 0
#define FONT_RESOURCE_TYPE "DFX_TTF"
#endif

namespace dfx {
namespace {

#if TARGET_OS_WIN32
// Returns the name of the temp file (used to unregister the resource) if
// successful, otherwise nullopt.
std::optional<std::string> InstallFontWin32(const char *resource_name) {
  HRSRC rsrc = FindResourceA(VSTGUI::GetInstance(), resource_name, FONT_RESOURCE_TYPE);
  if (rsrc == nullptr) {
    TRACE("FindResourceA failed");
    return {};
  }

  DWORD rsize = SizeofResource(VSTGUI::GetInstance(), rsrc);
  HGLOBAL data_load = LoadResource(VSTGUI::GetInstance(), rsrc);
  if (data_load == nullptr) {
    TRACE("LoadResource failed?");
    return {};
  }
  // (Note that this is not really a "lock" and there is no corresponding
  // unlock.)
  HGLOBAL data = LockResource(data_load);
  if (data == nullptr) {
    TRACE("LockResource failed??");
    return {};
  }

  // It would be nice if we could just do
  //  AddFontMemResourceEx(data, rsize, 0, &num_loaded);
  // but, while this successfully installs the font from memory, the
  // font is not enumerable and so it cannot be found by its name
  // when VSTGUI tries to create it. Weirdly there is no option to
  // do this for the memory version, but it is possible with the
  // version that reads from a file. So, write the resource data to
  // a temporary file and load it from there.
  
  char temp_path[MAX_PATH + 1] = {};
  if (!GetTempPathA(MAX_PATH, temp_path)) {
    TRACE("GetTempPathA failed");
    return {};
  }

  // Note that the temp path may not exist, or may not be writable;
  // it just comes from the environment. So this and the file operations
  // can definitely fail.
  char temp_filename[MAX_PATH + 15] = {};
  if (!GetTempFileNameA(temp_path, "dfx_font_", 0, temp_filename)) {
    TRACE("GetTempFileName failed");
    return {};
  }
  
  HANDLE temp_fh = CreateFileA((LPTSTR)temp_filename,
                               GENERIC_WRITE,
                               0,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
  if (temp_fh == INVALID_HANDLE_VALUE) {
    TRACE("CreateFile failed for temp file");
    return {};
  }

  // Write the data to the temp file.
  DWORD bytes_written = 0;
  if (!WriteFile(temp_fh, data, rsize, &bytes_written, nullptr)) {
    TRACE("WriteFile failed");
    return {};
  }
  if (bytes_written != rsize) {
    // (Sanity checking; WriteFile should have returned false.)
    TRACE("Incomplete WriteFile");
    return {};
  }

  if (!CloseHandle(temp_fh)) {
    TRACE("Couldn't close the temp file handle??");
    return {};
  }

  if (!AddFontResourceExA(temp_filename,
			  FONT_RESOURCE_FLAGS,
                          /* Reserved, must be zero */
                          0)) {
    TRACE("AddFontResourceEx failed");
    // If we didn't actually install the font, then try to remove the
    // temp file, as we won't try to clean up the resource later.
    (void)DeleteFileA(temp_filename);
    return {};
  }
  
  TRACE("Font successfully installed");
  return {(std::string)temp_filename};
}
#endif

// Platform-specific name of an embedded resource.
#ifdef TARGET_OS_MAC
  using ResourceRef = CFURLRef;
#elif TARGET_OS_WIN32
  using ResourceRef = const char *;
#else
  using ResourceRef = void;
#endif


// For cleaning up resources when the font factory is destroyed.
// Currently trivial on platforms other than Windows.
#ifdef TARGET_OS_WIN32
struct InstalledFontResource {
public:
  InstalledFontResource(const std::string &tempfile) : tempfile(tempfile) {}
  ~InstalledFontResource() {
    if (!RemoveFontResourceExA(tempfile.c_str(), FONT_RESOURCE_FLAGS, 0)) {
      TRACE("Failed to remove resource");
    } else {
      TRACE("Removed resource");
    }

    // Either way, also try removing the temp file.
    // 
    // (XXX: For some reason this doesn't always work! It looks like
    // the "system" lock on the file may not be immediately gone when
    // RemoveFontResourceExA returns? Maybe there is actually a race
    // where some CFontDesc instances outlive FontFactory? But it
    // works sometimes, and leaving some small files in the temp
    // directory is not the end of the world. Is there an equivalent
    // of "unlinked" files for windows, perhaps?)
    if (!DeleteFileA(tempfile.c_str())) {
      TRACE("Couldn't delete temp file");
    } else {
      TRACE("Deleted temp font file!");
    }
  }
  
  const std::string tempfile;
};
#else
using InstalledFontResource = int;
#endif

class FFImpl : public FontFactory {
public:
  FFImpl() {

  }
  
  ~FFImpl() override {
    // (unique pointers in installed vector clean up font resources)
  }

  // Try to register a font from its resource name.
  void RegisterFont(ResourceRef ref) {
#if TARGET_OS_MAC
    [[maybe_unused]] auto const success =
      CTFontManagerRegisterFontsForURL(fileURL, kCTFontManagerScopeProcess,
				       nullptr);
#elif TARGET_OS_WIN32
    std::optional<std::string> tempfile = InstallFontWin32(ref);
    if (tempfile.has_value()) {
      installed.emplace_back(
	  std::make_unique<InstalledFontResource>(tempfile.value()));
    } else {
      TRACE("InstallFontWin32 failed?");
    }
#else
    // (Impossible)
#endif
  }
  
  void InstallAllFonts() override {
    const std::lock_guard<std::mutex> ml(m);
    if (did_installation) return;
    did_installation = true;
    
#if TARGET_OS_MAC
    // load any fonts from our bundle resources to be accessible
    // locally within our component instance
    auto const pluginBundle =
      CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
    if (pluginBundle) {
      dfx::UniqueCFType const bundleResourcesDirURL =
        CFBundleCopyResourcesDirectoryURL(pluginBundle);
      if (bundleResourcesDirURL) {
        constexpr CFURLEnumeratorOptions options =
          kCFURLEnumeratorSkipInvisibles |
	  kCFURLEnumeratorSkipPackageContents |
          kCFURLEnumeratorDescendRecursively;
        dfx::UniqueCFType const dirEnumerator =
          CFURLEnumeratorCreateForDirectoryURL(kCFAllocatorDefault,
					       bundleResourcesDirURL.get(),
                                               options, nullptr);
        if (dirEnumerator) {
          CFURLRef fileURL = nullptr;
          while (CFURLEnumeratorGetNextURL(dirEnumerator.get(), &fileURL,
					   nullptr) ==
		 kCFURLEnumeratorSuccess) {
            if (fileURL) {
              if (CFURLHasDirectoryPath(fileURL)) {
                continue;
              }
              // XXX TODO: only call this for font files!
              RegisterFont(fileURL);
            }
          }
        }
      }
    }
#elif TARGET_OS_WIN32

    // TODO: VSTGUI does have direct support for this, but it uses
    // APIs that are only available in Windows 10 version 1703 (April
    // 2017). Once this version is widely used we can retire this
    // hack, but currently some 35% of users are on Windows 8 or
    // earlier!
    if (!EnumResourceNamesA(VSTGUI::GetInstance(),
			    FONT_RESOURCE_TYPE,
			    +[](HMODULE hModule,
				LPCTSTR type,
				char *name,
				LONG_PTR param) -> int {
				// MessageBoxA(nullptr, name, name, 0);
				((FFImpl*)param)->RegisterFont(name);
				// Continue enumeration.
				return true;
			      },
			    (LONG_PTR)this)) {
      // Note: this also fails if no resources are found, which would
      // be normal for plugins without embedded fonts.
      TRACE("EnumResourceNamesA failed (or no DFX_TTF)");
      return;
    }
    
#else

    // (maybe VSTGUI::IPlatformFont::getAllPlatformFontFamilies can assist?)
    #warning "implementation missing"

#endif
  }

  // Two ideas behind having this here:
  //  - Allows us to make sure the fonts are initialized (although it's
  //    somewhat redundant given that we can easily call InstallAllFonts
  //    in the editor's open() function)
  //  - Clarifies the lifetime of CFontDesc for custom fonts (shouldn't
  //    try to delete the font resource while the font is being used)
  //  - Allows us to extend to custom fonts that manage resources, like
  //    a bitmap font that has its own allocated storage.
  VSTGUI::SharedPointer<VSTGUI::CFontDesc> CreateVstGuiFont(
      float inFontSize, char const* inFontName) override {
    TRACE("CreateVstGuiFont");
    InstallAllFonts();
    return ::dfx::CreateVstGuiFontInternal(inFontSize, inFontName);
  }

  std::mutex m;
  bool did_installation = false;
  [[maybe_unused]]
  std::vector<std::unique_ptr<InstalledFontResource>> installed;
};

}  // namespace

std::unique_ptr<FontFactory> FontFactory::Create() {
  return std::make_unique<FFImpl>();
}

}  // namespace dfx

