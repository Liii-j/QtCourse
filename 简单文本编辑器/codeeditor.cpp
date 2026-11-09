#include "codeeditor.h"

#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>

// ------------------------------------------------------------- 行号栏 --
LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor)
    , m_editor(editor)
{
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

// ----------------------------------------------------------- CodeEditor --
CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_lineNumberArea->hide();

    m_background = palette().color(QPalette::Base);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);

    updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.left(), cr.top(),
                                  m_lineNumberVisible ? lineNumberAreaWidth() : 0,
                                  cr.height());
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    const int width = m_lineNumberVisible ? lineNumberAreaWidth() : 0;
    setViewportMargins(width, 0, 0, 0);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(cr.left(), cr.top(), width, cr.height());
    if (m_lineNumberVisible)
        m_lineNumberArea->raise();
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (!m_lineNumberVisible)
        return;

    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::setLineNumberVisible(bool visible)
{
    m_lineNumberVisible = visible;
    m_lineNumberArea->setVisible(visible);
    if (visible)
        m_lineNumberArea->raise();

    updateLineNumberAreaWidth();
    m_lineNumberArea->update();
}

void CodeEditor::refreshLineNumberArea()
{
    updateLineNumberAreaWidth();
    m_lineNumberArea->update();
}

void CodeEditor::setEditorBackgroundColor(const QColor &color)
{
    m_background = color;

    QPalette pal = palette();
    pal.setColor(QPalette::Base, color);
    // 深色系统主题下文字是白色的，换浅色背景时必须同时把文字调深，否则看不清
    pal.setColor(QPalette::Text, color.lightness() > 128 ? QColor(24, 24, 24)
                                                         : QColor(228, 228, 228));
    setPalette(pal);

    m_lineNumberArea->update();   // 行号栏底色跟着变
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);

    painter.fillRect(event->rect(), m_background.darker(106));   // 比编辑区略深一点
    painter.setPen(m_background.darker(125));
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());

    const bool light = m_background.lightness() > 128;
    painter.setPen(light ? QColor(120, 120, 120) : QColor(195, 195, 195));
    const int fontHeight = fontMetrics().height();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top, m_lineNumberArea->width() - 6, fontHeight,
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
