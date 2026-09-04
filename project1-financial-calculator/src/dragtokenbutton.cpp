#include "dragtokenbutton.h"

#include <QApplication>
#include <QDrag>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>

namespace
{
constexpr auto kMimeType = "application/x-financial-calculator-token";
}

DragTokenButton::DragTokenButton(QWidget *parent) : QPushButton(parent) {}

QString DragTokenButton::token() const
{
    return m_token.isEmpty() ? text() : m_token;
}

void DragTokenButton::setToken(const QString &token)
{
    m_token = token;
}

void DragTokenButton::mousePressEvent(QMouseEvent *event)
{
    // 记录按下位置，用于判断后续移动是否达到拖拽阈值。
    if (event->button() == Qt::LeftButton)
    {
        m_pressPosition = event->position().toPoint();
    }
    QPushButton::mousePressEvent(event);
}

void DragTokenButton::mouseMoveEvent(QMouseEvent *event)
{
    // 鼠标移动距离不足时仍交给 QPushButton 处理普通点击。
    if (!(event->buttons() & Qt::LeftButton) ||
        (event->position().toPoint() - m_pressPosition).manhattanLength() <
            QApplication::startDragDistance())
    {
        QPushButton::mouseMoveEvent(event);
        return;
    }

    // 通过自定义 MIME 类型传递按钮代表的数字或运算符。
    auto *mimeData = new QMimeData;
    mimeData->setData(kMimeType, token().toUtf8());
    mimeData->setText(token());

    // 生成跟随鼠标显示的拖拽预览，提升拖拽操作的可见反馈。
    QPixmap preview(56, 56);
    preview.fill(Qt::transparent);
    QLabel label;
    label.setFixedSize(preview.size());
    label.setAlignment(Qt::AlignCenter);
    label.setText(token());
    label.setStyleSheet(QStringLiteral(
        "background:#2563eb;color:white;border-radius:12px;font-size:24px;font-weight:700;"));
    label.render(&preview);

    auto *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(preview);
    drag->setHotSpot(QPoint(28, 28));
    drag->exec(Qt::CopyAction);
}
