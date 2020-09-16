/**
 * @file tools/capgrep/src/HexView.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Widget to display a hex dump.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "HexView.h"

// Ignore warnings
#include <PushIgnore.h>

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollArea>

// Stop ignoring warnings
#include <PopIgnore.h>

HexView::HexView(QWidget *p) : QWidget(p), mSelectionActive(false) {
  // Left Top Right Bottom
  mMargin = QMargins(0, 0, 0, 0);
  mLinePadding = QMargins(0, 0, 0, 0);
  mAddrPadding = QMargins(5, 0, 5, 0);
  mBytePadding = QMargins(2, 0, 2, 0);
  mBytesPadding = QMargins(3, 0, 3, 0);
  mAsciiPadding = QMargins(5, 0, 5, 0);
  mCharPadding = QMargins(2, 0, 2, 0);

  mByteSpacing = 5;
  mCharSpacing = 0;

  mSelectionStart = -1;
  mSelectionEnd = -1;

  mFont = QFont("Monospace", 10);
  mFontColor = QPen(Qt::black);
  mAltFontColor = QPen(Qt::blue);
  mBackground = QBrush(Qt::white);
  mAddrBackground = QBrush(Qt::lightGray);

  HexViewPaintState state;
  initState(state);

  setMinimumWidth(state.minWidth);
  setFocusPolicy(Qt::ClickFocus);
}

void HexView::initState(HexViewPaintState &state) const {
  QFontMetrics metrics(mFont);

#if QT_VERSION >= 0x050B00
  state.addrWidth = mMargin.left() + mLinePadding.left();
  state.addrWidth +=
      mAddrPadding.left() + metrics.horizontalAdvance("00000000");
  state.addrWidth += mAddrPadding.right();

  state.byteWidth = mBytePadding.left() + metrics.horizontalAdvance("00");
  state.byteWidth += mBytePadding.right();

  state.charWidth = mCharPadding.left() + metrics.horizontalAdvance("0");
  state.charWidth += mCharPadding.right();
#else
  state.addrWidth = mMargin.left() + mLinePadding.left();
  state.addrWidth += mAddrPadding.left() + metrics.width("00000000");
  state.addrWidth += mAddrPadding.right();

  state.byteWidth = mBytePadding.left() + metrics.width("00");
  state.byteWidth += mBytePadding.right();

  state.charWidth = mCharPadding.left() + metrics.width("0");
  state.charWidth += mCharPadding.right();
#endif

  state.binaryX = state.addrWidth + 1 + mBytesPadding.left();
  state.lineHeight = metrics.height();

  state.ascent = metrics.ascent();
  state.sz = mData.size();

  state.x = mMargin.left() + mLinePadding.left();
  state.y = mMargin.top() + mLinePadding.top();

  // Calculate the width with only one byte.
  state.bytesPerLine = 1;
  state.asciiLine = state.binaryX + state.byteWidth + mBytesPadding.right();
  state.asciiX = state.asciiLine + 1 + mAsciiPadding.left();
  state.endLine = state.asciiX + state.charWidth + mAsciiPadding.right();
  state.minWidth = state.endLine + 1;

  // Calculate how much width is added for a single byte.
  int widthGain =
      state.byteWidth + mByteSpacing + state.charWidth + mCharSpacing;

  // Calculate how many bytes can fit the width of the widget.
  state.bytesPerLine = (width() - (state.endLine + 1)) / widthGain + 1;

  // Ensure we have at least one byte per line.
  if (state.bytesPerLine < 1) state.bytesPerLine = 1;

  // Recalculate the values now that we know how many bytes are on a line.
  int bytesWidth = (state.bytesPerLine * state.byteWidth) +
                   ((state.bytesPerLine - 1) * mByteSpacing);
  int asciiWidth = (state.bytesPerLine * state.charWidth) +
                   ((state.bytesPerLine - 1) * mCharSpacing);

  state.asciiLine = state.binaryX + bytesWidth + mBytesPadding.right();
  state.asciiX = state.asciiLine + 1 + mAsciiPadding.left();
  state.endLine = state.asciiX + asciiWidth + mAsciiPadding.right();
}

void HexView::paintEvent(QPaintEvent *evt) {
  int rectTop = evt->rect().top();
  int rectBottom = evt->rect().bottom();

  HexViewPaintState state;
  initState(state);

  QPainter p(this);

  p.fillRect(evt->rect(), mBackground);
  p.setClipRect(evt->rect());
  p.setFont(mFont);

  // Draw the address background
  p.setPen(Qt::NoPen);
  p.setBrush(mAddrBackground);
  p.drawRect(0, 0, state.addrWidth, height());

  int line = mLinePadding.bottom();
  line += state.lineHeight + mLinePadding.top();

  int start = (rectTop - state.y) / line;

  state.y += start * line;
  start *= state.bytesPerLine;

  for (int i = start; i < state.sz; i += state.bytesPerLine) {
    if (state.y > rectBottom) break;

    // Draw the address background
    if (i < 0) {
      p.setPen(Qt::NoPen);
      p.setBrush(Qt::red);
      p.drawRect(state.addrWidth + 1, state.y, width() - (state.addrWidth + 1),
                 mLinePadding.top() + state.lineHeight + mLinePadding.bottom());
    }

    paintLine(state, p, i);

    state.y += mLinePadding.bottom();
    state.y += state.lineHeight + mLinePadding.top();
  }

  // Draw the address divider line
  p.setPen(mFontColor);
  p.drawLine(state.addrWidth, 0, state.addrWidth, height());

  // Draw the ascii divider line
  p.setPen(mFontColor);
  p.drawLine(state.asciiLine, 0, state.asciiLine, height());

  // Draw the end divider line
  p.setPen(mFontColor);
  p.drawLine(state.endLine, 0, state.endLine, height());
}

void HexView::paintLine(const HexViewPaintState &state, QPainter &p, int addr) {
  QString addrString = tr("%1").arg(addr, 8, 16, QLatin1Char('0'));
  if (true) addrString = addrString.toUpper();

  p.setPen(mFontColor);
  p.drawText(state.x + mAddrPadding.left(),
             state.y + mAddrPadding.top() + state.ascent, addrString);

  QString ascii;

  int start = mSelectionStart;
  int end = mSelectionEnd;

  if (mSelectionEnd < mSelectionStart) {
    start = mSelectionEnd;
    end = mSelectionStart;
  }

  bool haveSelection = start >= 0 && end >= 0;

  for (int i = 0; (addr + i) < state.sz && i < state.bytesPerLine; i++) {
    int realAddr = addr + i;

    QString byte =
        tr("%1").arg((quint8)mData[realAddr], 2, 16, QLatin1Char('0'));
    if (true) byte = byte.toUpper();

    ascii.clear();
    if (mData.at(realAddr) >= 32 && mData.at(realAddr) <= 126)
      ascii.append(mData.at(realAddr));
    else
      ascii.append('.');

    bool noExtend = (end == realAddr) ||
                    (realAddr % state.bytesPerLine) == (state.bytesPerLine - 1);

    if (haveSelection && start <= realAddr && end >= realAddr) {
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(65, 141, 212));
      p.drawRect(state.binaryX + (i * state.byteWidth) + (i * mByteSpacing),
                 state.y + mBytesPadding.top(),
                 state.byteWidth + (noExtend ? 0 : mByteSpacing),
                 state.lineHeight);
    }

    p.setPen(i % 2 ? mAltFontColor : mFontColor);
    p.drawText(
        state.binaryX + (i * state.byteWidth) + (i * mByteSpacing) +
            mBytePadding.left(),
        state.y + mBytesPadding.top() + mBytePadding.top() + state.ascent,
        byte);

    // Selected
    if (haveSelection && start <= realAddr && end >= realAddr) {
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(65, 141, 212));
      p.drawRect(state.asciiX + (i * state.charWidth) + (i * mCharSpacing),
                 state.y + mAsciiPadding.top(),
                 state.charWidth + (noExtend ? 0 : mCharSpacing),
                 state.lineHeight);
    }

    p.setPen(mFontColor);
    p.drawText(
        state.asciiX + mCharPadding.left() + (i * state.charWidth) +
            (i * mCharSpacing),
        state.y + mAsciiPadding.top() + mCharPadding.top() + state.ascent,
        ascii);
  }
}

int HexView::pointToAddr(const QPoint &pt, bool *isAscii) const {
  if (pt.x() < mMargin.left() || pt.y() < mMargin.top()) return -1;

  HexViewPaintState state;
  initState(state);

  state.x = pt.x();
  state.y = pt.y() - mMargin.top();

  int lineHeight = mLinePadding.top() + state.lineHeight;
  lineHeight += mLinePadding.bottom();

  int line = state.y / lineHeight;

  // Adjust the line Y
  state.y %= lineHeight;

  int maxLines = mData.size() / state.bytesPerLine;
  if (mData.size() % state.bytesPerLine) maxLines++;

  if (line >= maxLines) return -1;

  if (state.x < state.binaryX) return -1;

  if (state.x < state.asciiLine)  // Possible byte selection
  {
    int top = mBytesPadding.top() + mBytePadding.top();
    if (state.y < top) return -1;

    int bottom = top + state.lineHeight;
    if (state.y >= bottom) return -1;

    state.x -= state.binaryX;

    int byteWidth = state.byteWidth + mByteSpacing;
    int index = state.x / byteWidth;

    state.x %= byteWidth;

    /*
    if( state.x < mBytePadding.left() )
        return -1;
    if( state.x >= (state.byteWidth - mBytePadding.right()) )
        return -1;
    */

    int currentPos = (state.bytesPerLine * line) + index;

    if (mData.size() <= currentPos) return -1;

    if (isAscii) *isAscii = false;

    return currentPos;
  } else  // Possible ASCII selection
  {
  }

  return -1;
}

void HexView::mouseMoveEvent(QMouseEvent *evt) {
  if (!mSelectionActive) {
    QWidget::mouseMoveEvent(evt);

    return;
  }

  mSelectionEnd = pointToAddr(evt->pos());

  scrollToOffset(mSelectionEnd);
  repaint();

  emit selectionChanged();
}

void HexView::mousePressEvent(QMouseEvent *evt) {
  if (evt->button() != Qt::LeftButton) {
    QWidget::mousePressEvent(evt);

    return;
  }

  mSelectionActive = true;

  if (evt->modifiers().testFlag(Qt::ShiftModifier)) {
    mSelectionEnd = pointToAddr(evt->pos());
  } else {
    mSelectionStart = pointToAddr(evt->pos());
    mSelectionEnd = mSelectionStart;
  }

  repaint();

  emit selectionChanged();
}

void HexView::mouseReleaseEvent(QMouseEvent *evt) {
  if (evt->button() != Qt::LeftButton) {
    QWidget::mouseReleaseEvent(evt);

    return;
  }

  mSelectionEnd = pointToAddr(evt->pos());
  mSelectionActive = false;

  repaint();

  emit selectionChanged();
}

void HexView::setData(const QByteArray &d) {
  mData = d;
  mSelectionStart = -1;
  mSelectionEnd = -1;

  HexViewPaintState state;
  initState(state);

  int lines = state.sz / state.bytesPerLine;
  if (state.sz % state.bytesPerLine != 0) lines++;

  state.y +=
      (mLinePadding.bottom() + state.lineHeight + mLinePadding.top()) * lines;

  setMinimumHeight(state.y);

  repaint();

  emit selectionChanged();
}

void HexView::setSelection(int start, int stop) {
  mSelectionStart = start;
  mSelectionEnd = stop;

  repaint();

  emit selectionChanged();
}

int HexView::startOffset() const {
  if (mSelectionEnd < mSelectionStart) return mSelectionEnd;

  return mSelectionStart;
}

int HexView::stopOffset() const {
  if (mSelectionEnd < mSelectionStart) return mSelectionStart;

  return mSelectionEnd;
}

void HexView::keyPressEvent(QKeyEvent *evt) {
  HexViewPaintState state;
  initState(state);

  int key = evt->key();

  if (key != Qt::Key_Up && key != Qt::Key_Down && key != Qt::Key_Left &&
      key != Qt::Key_Right) {
    QWidget::keyPressEvent(evt);

    return;
  }

  int start = mSelectionStart;

  if (evt->modifiers().testFlag(Qt::ShiftModifier)) start = mSelectionEnd;

  switch (key) {
    case Qt::Key_Up: {
      if (start >= state.bytesPerLine) start -= state.bytesPerLine;

      break;
    }
    case Qt::Key_Down: {
      if ((start + state.bytesPerLine) < state.sz) start += state.bytesPerLine;

      break;
    }
    case Qt::Key_Left: {
      if (start > 0) start--;

      break;
    }
    case Qt::Key_Right: {
      if (start < (state.sz - 1)) start++;

      break;
    }
    default: {
      return;
    }
  }

  if (evt->modifiers().testFlag(Qt::ShiftModifier)) {
    mSelectionEnd = start;
  } else {
    mSelectionStart = start;
    mSelectionEnd = start;
  }

  scrollToOffset(start);
  repaint();

  emit selectionChanged();
}

void HexView::keyReleaseEvent(QKeyEvent *evt) {
  int key = evt->key();
  if (key != Qt::Key_Up && key != Qt::Key_Down && key != Qt::Key_Left &&
      key != Qt::Key_Right) {
    QWidget::keyPressEvent(evt);

    return;
  }
}

void HexView::scrollToOffset(int offset) {
  QObject *p = parent();
  if (!p) return;

  QScrollArea *area = qobject_cast<QScrollArea *>(p);

  while (!area) {
    p = p->parent();
    if (!p) return;

    area = qobject_cast<QScrollArea *>(p);
  }

  HexViewPaintState state;
  initState(state);

  int line = offset / state.bytesPerLine;

  if (line <= 0) {
    area->ensureVisible(0, 0, 0, 0);

    return;
  }

  int top =
      state.y +
      ((mLinePadding.bottom() + state.lineHeight + mLinePadding.top()) * line);
  int bottom =
      top + mLinePadding.bottom() + state.lineHeight + mLinePadding.top() - 1;

  area->ensureVisible(0, top, 0, 0);
  area->ensureVisible(0, bottom, 0, 0);
}
