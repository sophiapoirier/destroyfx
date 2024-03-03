 /*------------------------------------------------------------------------
Destroy FX Library is a collection of foundation code 
for creating audio processing plug-ins.  
Copyright (C) 2002-2024  Sophia Poirier and Tom Murphy VII

This file is part of the Destroy FX Library (version 1.0).

Destroy FX Library is free software:  you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 2 of the License, or 
(at your option) any later version.

Destroy FX Library is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Destroy FX Library.  If not, see <http://www.gnu.org/licenses/>.

To contact the author, use the contact form at http://destroyfx.org
------------------------------------------------------------------------*/

#include "dfxgui-fontfactory.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "dfx-base.h"
#include "dfxmisc.h"

#if TARGET_OS_WIN32
#include <windows.h>
// Some platform-specific vstgui peeking in order to load custom fonts.
// (XXX: GetInstance can probably now come from DfxGuiEditor, which would
// be a little cleaner?)
#include "lib/platform/win32/win32support.h"
#endif

#if TARGET_OS_MAC
#if !__OBJC__
  #error "you must compile the version of this file with a .mm filename extension, not this file"
#elif !__has_feature(objc_arc)
  #error "you must compile this file with Automatic Reference Counting (ARC) enabled"
#endif
#import <AppKit/NSFont.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CTFontManager.h>
#endif

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
    return {};
  }

  DWORD rsize = SizeofResource(VSTGUI::GetInstance(), rsrc);
  HGLOBAL data_load = LoadResource(VSTGUI::GetInstance(), rsrc);
  if (data_load == nullptr) {
    return {};
  }
  // (Note that this is not really a "lock" and there is no corresponding
  // unlock.)
  HGLOBAL data = LockResource(data_load);
  if (data == nullptr) {
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
    return {};
  }

  // Note that the temp path may not exist, or may not be writable;
  // it just comes from the environment. So this and the file operations
  // can definitely fail.
  char temp_filename[MAX_PATH + 15] = {};
  if (!GetTempFileNameA(temp_path, "dfx_font_", 0, temp_filename)) {
    return {};
  }

  HANDLE temp_fh = CreateFileA(static_cast<LPTSTR>(temp_filename),
                               GENERIC_WRITE,
                               0,
                               nullptr,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);
  if (temp_fh == INVALID_HANDLE_VALUE) {
    return {};
  }

  // Write the data to the temp file.
  DWORD bytes_written = 0;
  if (!WriteFile(temp_fh, data, rsize, &bytes_written, nullptr)) {
    return {};
  }
  if (bytes_written != rsize) {
    // (Consistency checking; WriteFile should have returned false.)
    return {};
  }

  if (!CloseHandle(temp_fh)) {
    return {};
  }

  if (!AddFontResourceExA(temp_filename,
                          FONT_RESOURCE_FLAGS,
                          /* Reserved, must be zero */
                          0)) {
    // If we didn't actually install the font, then try to remove the
    // temp file, as we won't try to clean up the resource later.
    std::ignore = DeleteFileA(temp_filename);
    return {};
  }

  return {std::string(temp_filename)};
}
#endif  // TARGET_OS_WIN32

// Platform-specific name of an embedded resource.
#if TARGET_OS_MAC
  using ResourceRef = CFURLRef;
#elif TARGET_OS_WIN32
  using ResourceRef = const char *;
#else
  using ResourceRef = void;
#endif


// For cleaning up resources when the font factory is destroyed.
// Currently trivial on platforms other than Windows.
#if TARGET_OS_WIN32
struct InstalledFontResource {
public:
  explicit InstalledFontResource(const std::string &tempfile) : tempfile(tempfile) {}
  ~InstalledFontResource() {
    if (!RemoveFontResourceExA(tempfile.c_str(), FONT_RESOURCE_FLAGS, 0)) {
      // Failed, but still try to remove the temp file...
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
      // Failed, but not much we can do...
    }
  }

  InstalledFontResource(InstalledFontResource const&) = delete;
  InstalledFontResource(InstalledFontResource&&) = delete;
  InstalledFontResource& operator=(InstalledFontResource const&) = delete;
  InstalledFontResource& operator=(InstalledFontResource&&) = delete;

private:
  const std::string tempfile;
};
#endif

class FFImpl : public FontFactory {
public:
  FFImpl() {
    InstallAllFonts();
  }

  // Try to register a font from its resource name.
  void RegisterFont(ResourceRef ref) {
#if TARGET_OS_MAC
    [[maybe_unused]] auto const success =
      CTFontManagerRegisterFontsForURL(ref, kCTFontManagerScopeProcess,
                                       nullptr);
#elif TARGET_OS_WIN32
    std::optional<std::string> tempfile = InstallFontWin32(ref);
    if (tempfile.has_value()) {
      installed.emplace_back(
          std::make_unique<InstalledFontResource>(tempfile.value()));
    }
#else
    // (Impossible)
#endif
  }

  // Called during constructor.
  void InstallAllFonts() {
#if TARGET_OS_MAC
    // Register any fonts located within our plugin bundle resources.
    // The registration occurs locally for the process, and thus need
    // only occur once and requires no clean up, as that will happen
    // automatically upon process exit.  All current and future
    // instances of the plugin in that process will have access to the
    // fonts registered here.
    static std::once_flag once;
    std::call_once(once, [this] {
      auto const pluginBundle =
        CFBundleGetBundleWithIdentifier(CFSTR(PLUGIN_BUNDLE_IDENTIFIER));
      assert(pluginBundle);
      if (pluginBundle) {
        auto const bundleResourcesDirURL =
          dfx::MakeUniqueCFType(CFBundleCopyResourcesDirectoryURL(pluginBundle));
        if (bundleResourcesDirURL) {
          constexpr CFURLEnumeratorOptions options =
            kCFURLEnumeratorSkipInvisibles |
            kCFURLEnumeratorSkipPackageContents |
            kCFURLEnumeratorDescendRecursively;
          auto const dirEnumerator =
            dfx::MakeUniqueCFType(CFURLEnumeratorCreateForDirectoryURL(kCFAllocatorDefault,
                                                                       bundleResourcesDirURL.get(),
                                                                       options, nullptr));
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
    });
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
                                reinterpret_cast<FFImpl*>(param)->RegisterFont(name);
                                // Continue enumeration.
                                return true;
                              },
                            reinterpret_cast<LONG_PTR>(this))) {
      // Note: this also "fails" if no resources of type DFX_TTF are found,
      // which would be normal for plugins without embedded fonts.
      return;
    }

#else

    #warning "implementation missing"

#endif
  }

  // Two ideas behind having this here:
  //  - Allows us to make sure the fonts are initialized before we
  //    try creating any.
  //  - Clarifies the lifetime of CFontDesc for custom fonts (shouldn't
  //    try to delete the font resource while the font is being used).
  //  - Allows us to extend to custom fonts that manage resources, like
  //    a bitmap font that has its own allocated storage.
  VSTGUI::SharedPointer<VSTGUI::CFontDesc> CreateVstGuiFont(
      float inFontSize, char const* inFontName) override {
    if (inFontName) {
      return VSTGUI::makeOwned<VSTGUI::CFontDesc>(inFontName, inFontSize);
    } else {
      auto const fontDesc = VSTGUI::makeOwned<VSTGUI::CFontDesc>(*VSTGUI::kSystemFont);
      fontDesc->setSize(inFontSize);
#if TARGET_OS_MAC
      if (auto const fontName = [NSFont systemFontOfSize:NSFont.systemFontSize].displayName) {
        fontDesc->setName(fontName.UTF8String);
      }
#endif
      return fontDesc;
    }
  }

private:
#if TARGET_OS_WIN32
  std::vector<std::unique_ptr<InstalledFontResource>> installed;
#endif
};

}  // namespace

std::unique_ptr<FontFactory> FontFactory::Create() {
  return std::make_unique<FFImpl>();
}

}  // namespace dfx
