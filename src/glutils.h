/*
 * Copyright (c) 2004-2017  Daniel Elstner  <daniel.kitta@gmail.com>
 *
 * This file is part of Somato.
 *
 * Somato is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Somato is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Somato.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOMATO_GLUTILS_H_INCLUDED
#define SOMATO_GLUTILS_H_INCLUDED

#include "gltypes.h"
#include "meshtypes.h"

#include <glib.h>
#include <glibmm/ustring.h>
#include <gdkmm/glcontext.h>

#include <type_traits>
#include <utility>
#include <cstddef>

#include <epoxy/gl.h>

namespace GL
{

constexpr const char* log_domain = "OpenGL";

/* Record of available GL extensions and implementation limits.
 */
struct Extensions
{
  int   version                    = 0;
  bool  is_gles                    = false;
  bool  debug                      = false;
  bool  debug_output               = false;
  bool  geometry_shader            = false;
  bool  texture_border_clamp       = false;
  bool  texture_filter_anisotropic = false;
  bool  texture_gather             = false;
  float max_anisotropy             = 0.;

  // Query GL extensions after initial context setup.
  static void query(bool use_es, int major, int minor)
    { instance_.query_(use_es, 10 * major + minor); }

  friend inline const Extensions& extensions();

private:
  void query_(bool use_es, int ver);

  static Extensions instance_;
};

/* Access GL extensions record.
 */
inline const Extensions& extensions() { return Extensions::instance_; }

/* Exception class for errors reported by glGetError() and other OpenGL
 * failure conditions.
 */
class Error : public Gdk::GLError
{
private:
  unsigned int gl_code_;

public:
  explicit Error(unsigned int error_code);
  explicit Error(const Glib::ustring& message, unsigned int error_code = 0);
  virtual ~Error() noexcept;

  Error(const Error& other) = default;
  Error& operator=(const Error& other) = default;

  unsigned int gl_code() const { return gl_code_; }

  static void check();                // throw if glGetError() != GL_NO_ERROR
  static void fail() G_GNUC_NORETURN; // like check() but always throws

  static void throw_if_fail(bool condition) { if (G_UNLIKELY(!condition)) fail(); }
};

class FramebufferError : public Error
{
public:
  explicit FramebufferError(unsigned int error_code);
  virtual ~FramebufferError() noexcept;
};

/* Scoped glMapBufferRange().
 */
class ScopedMapBuffer
{
public:
  ScopedMapBuffer(unsigned int target, std::size_t offset,
                  std::size_t length, unsigned int access)
    : data_ {map_checked(target, offset, length, access)}, target_ {target}
  {}
  ~ScopedMapBuffer() { if (data_) unmap_checked(target_); }

  ScopedMapBuffer(const ScopedMapBuffer& other) = delete;
  ScopedMapBuffer& operator=(const ScopedMapBuffer& other) = delete;

  void* data() const { return data_; }
  bool unmap() { data_ = nullptr; return unmap_checked(target_); }

private:
  static void* map_checked(unsigned int target, std::size_t offset,
                           std::size_t length, unsigned int access);
  static bool unmap_checked(unsigned int target);

  void*        data_;
  unsigned int target_;
};

/* Encapsulate access to a temporarily mapped buffer object.
 */
template <typename Op>
inline bool access_mapped_buffer(unsigned int target, std::size_t offset,
                                 std::size_t length, unsigned int access,
                                 Op operation)
{
  ScopedMapBuffer buffer {target, offset, length, access};

  if (void *const data = buffer.data())
  {
    operation(data);
    return buffer.unmap();
  }
  return false;
}

/* Load a compressed 2D texture image from a KTX file memory buffer.
 */
void tex_image_from_ktx(const guint32* ktx, unsigned int ktx_size);

template <typename T> constexpr GLenum attrib_type_;

template <> constexpr GLenum attrib_type_<GLbyte>     = GL_BYTE;
template <> constexpr GLenum attrib_type_<GLubyte>    = GL_UNSIGNED_BYTE;
template <> constexpr GLenum attrib_type_<GLshort>    = GL_SHORT;
template <> constexpr GLenum attrib_type_<GLushort>   = GL_UNSIGNED_SHORT;
template <> constexpr GLenum attrib_type_<GLint>      = GL_INT;
template <> constexpr GLenum attrib_type_<GLuint>     = GL_UNSIGNED_INT;
template <> constexpr GLenum attrib_type_<GLfloat>    = GL_FLOAT;
template <> constexpr GLenum attrib_type_<Packed2i16> = GL_SHORT;
template <> constexpr GLenum attrib_type_<Packed4u8>  = GL_UNSIGNED_BYTE;
template <> constexpr GLenum attrib_type_<Int_2_10_10_10_rev> = GL_INT_2_10_10_10_REV;

template <typename T> constexpr GLenum attrib_type =
    attrib_type_<std::remove_all_extents_t<std::remove_reference_t<T>>>;

template <typename T>           constexpr int attrib_size_ = 1;
template <typename T, size_t N> constexpr int attrib_size_<T[N]> = N;

template <> constexpr int attrib_size_<Packed2i16> = 2;
template <> constexpr int attrib_size_<Packed4u8>  = 4;
template <> constexpr int attrib_size_<Int_2_10_10_10_rev> = 4;

template <typename T>
constexpr int attrib_size = attrib_size_<std::remove_reference_t<T>>;

/* Return whether the user requested OpenGL debug mode.
 */
bool debug_mode_requested();

/* Set the label of a GL object if the debug extension is available.
 */
void set_object_label(GLenum identifier, GLuint name, const char* label);

/* Convert a VBO offset to a pointer.
 */
template <typename T = char>
constexpr void* buffer_offset(std::size_t offset)
{
  return static_cast<T*>(0) + offset;
}

} // namespace GL

#endif // !SOMATO_GLUTILS_H_INCLUDED
