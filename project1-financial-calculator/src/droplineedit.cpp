#include "droplineedit.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

namespace
{
constexpr auto kMimeType = "application/x-financial-calculator-token";
}

DropLineEdit::DropLineEdit(QWidget *parent) : QLineEdit(parent)
{
    // 输入框作为拖拽目标，允许接收数字和运算符。
    setAcceptDrops(true);
}

QString DropLineEdit::tokenMimeType()
{
    return QString::fromLatin1(kMimeType);
}

void DropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    // 只有本程序定义的 MIME 数据才接受进入输入框。
    if (event->mimeData()->hasFormat(kMimeType))
    {
        event->acceptProposedAction();
        return;
    }
    QLineEdit::dragEnterEvent(event);
}

void DropLineEdit::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat(kMimeType))
    {
        QLineEdit::dropEvent(event);
        return;
    }

    // 释放鼠标后取出 token，追加到当前光标位置并通知主窗口。
    const QString token = QString::fromUtf8(event->mimeData()->data(kMimeType));
    insert(token);
    emit tokenDropped(token);
    event->acceptProposedAction();
}
