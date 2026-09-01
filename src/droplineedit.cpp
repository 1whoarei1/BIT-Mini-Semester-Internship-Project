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
    setAcceptDrops(true);
}

QString DropLineEdit::tokenMimeType()
{
    return QString::fromLatin1(kMimeType);
}

void DropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
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

    const QString token = QString::fromUtf8(event->mimeData()->data(kMimeType));
    insert(token);
    emit tokenDropped(token);
    event->acceptProposedAction();
}
