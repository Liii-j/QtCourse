#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QColor>
#include <QPlainTextEdit>
#include <QWidget>

class CodeEditor;

// 行号栏：贴在编辑区左侧的窄条，绘制内容由 CodeEditor 提供
class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *m_editor = nullptr;
};

// 带行号栏的纯文本编辑区
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setLineNumberVisible(bool visible);
    bool isLineNumberVisible() const { return m_lineNumberVisible; }

    void setEditorBackgroundColor(const QColor &color);
    QColor editorBackgroundColor() const { return m_background; }

    // 字体变化后重新计算行号栏宽度
    void refreshLineNumberArea();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_lineNumberVisible = false;
    QColor m_background;
};

#endif // CODEEDITOR_H
