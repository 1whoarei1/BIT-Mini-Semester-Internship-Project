#include "dropzonewidget.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QStyle>
#include <QUrl>

DropZoneWidget::DropZoneWidget(QWidget *parent) : QLabel(parent)
{
    // QLabel 默认不接收拖放事件，必须显式开启后虚函数重写才会被调用。
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setProperty("dragActive", false);
}

void DropZoneWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // 只接收一个本地普通文件，进入拖拽区时通过动态属性触发 QSS 高亮。
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()
        && QFileInfo(urls.first().toLocalFile()).isFile())
    {
        event->acceptProposedAction();
        setDragActive(true);
        return;
    }
    event->ignore();
}

void DropZoneWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    // 光标离开区域后立即恢复普通样式，避免动态属性一直停留在高亮状态。
    setDragActive(false);
    event->accept();
}

void DropZoneWidget::dropEvent(QDropEvent *event)
{
    setDragActive(false);
    // 与 dragEnterEvent 再做一次相同校验，不能假设拖放期间 MIME 数据始终不变。
    const QList<QUrl> urls = event->mimeData()->urls();
    if (urls.size() != 1 || !urls.first().isLocalFile())
    {
        event->ignore();
        return;
    }

    const QString filePath = urls.first().toLocalFile();
    if (!QFileInfo(filePath).isFile())
    {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    emit fileDropped(filePath);
}

void DropZoneWidget::setDragActive(bool active)
{
    setProperty("dragActive", active);
    // 动态属性改变后重新 polish，令 QSS 中 [dragActive="true"] 选择器立即生效。
    style()->unpolish(this);
    style()->polish(this);
    update();
}
