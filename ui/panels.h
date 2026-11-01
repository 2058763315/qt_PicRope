#pragma once
// =====================================================================
//  ui/panels.h — 图片篮子 / 文字信封
//  对应原 PicRope 的 BasketWindow 与 TextsWindow：
//  篮子用于暂存与批量挂图，信封用于搜索、查看与复制文字。
// =====================================================================

#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QStringList>

class QVBoxLayout;
class QPushButton;
class QListWidget;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QHideEvent;
class Gallery;
class TextCollection;

// 圆角面板基类：无系统边框、投影、可拖动标题区
class PanelWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PanelWindow(const QString &title, QWidget *parent = nullptr);

    // anchor 为工具托板（三个按钮）的屏幕矩形：面板贴在它左侧显示
    void showPanel(const QRect &anchor);

signals:
    void hidden();

protected:
    QVBoxLayout *contentLayout() const { return content_; }

    void paintEvent(QPaintEvent *) override;
    void hideEvent(QHideEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QVBoxLayout *content_ = nullptr;
    QPoint dragOffset_;
    bool dragging_ = false;
};

// 图片篮子：待挂 / 全部，两种视图，多选后挂上绳子
class BasketWindow : public PanelWindow
{
    Q_OBJECT
public:
    explicit BasketWindow(Gallery *gallery, QWidget *parent = nullptr);

    void refresh();

signals:
    void hangRequested(const QStringList &paths);

private:
    void setTab(int tab);
    void updateSummary();

    Gallery *gallery_;
    QPushButton *pendingTab_ = nullptr;
    QPushButton *allTab_ = nullptr;
    QListWidget *list_ = nullptr;
    QLabel *summary_ = nullptr;
    QPushButton *hangBtn_ = nullptr;
    int tab_ = 0;
    bool refreshing_ = false;
};

// 文字信封：搜索、查看全文、复制与删除
class TextsWindow : public PanelWindow
{
    Q_OBJECT
public:
    explicit TextsWindow(TextCollection *texts, QWidget *parent = nullptr);

    void refresh();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updatePreview();
    void copySelected();
    void deleteSelected();
    void flashStatus(const QString &text);

    TextCollection *texts_;
    QLineEdit *search_ = nullptr;
    QListWidget *list_ = nullptr;
    QPlainTextEdit *preview_ = nullptr;
    QLabel *status_ = nullptr;
    QPushButton *copyBtn_ = nullptr;
    QPushButton *deleteBtn_ = nullptr;
    bool refreshing_ = false;
};
