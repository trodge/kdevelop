/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>
  Copyright 2006 Hamish Rodda <rodda@kde.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PP_MACRO_EXPANDER_H
#define PP_MACRO_EXPANDER_H

#include <QList>
#include <QHash>
#include <QString>

#include "pp-macro.h"
#include "pp-stream.h"
#include "pp-scanner.h"

class pp_frame
{
public:
  pp_frame (pp_macro* __expandingMacro, const QList<QString>& __actuals);

  pp_macro* expandingMacro;
  QList<QString> actuals;
};

class pp_macro_expander
{
public:
  pp_macro_expander(QHash<QString, pp_macro*>& environment, pp_frame* frame = 0);

  QString resolve_formal(const QString& name);

  int linesSkipped() const;
  int generatedLines() const;

  void operator()(Stream& input, Stream& output);

  void skip_argument_variadics (const QList<QString>& __actuals, pp_macro *__macro,
                                Stream& input, Stream& output);

private:
  QHash<QString, pp_macro*>& m_environment;
  pp_frame* m_frame;

  pp_skip_number skip_number;
  pp_skip_identifier skip_identifier;
  pp_skip_string_literal skip_string_literal;
  pp_skip_char_literal skip_char_literal;
  pp_skip_argument skip_argument;
  pp_skip_comment_or_divop skip_comment_or_divop;
  pp_skip_blanks skip_blanks;

  int m_lines;
  int m_generatedLines;
};

#endif // PP_MACRO_EXPANDER_H

// kate: indent-width 2;
