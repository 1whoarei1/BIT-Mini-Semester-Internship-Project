#pragma once

#include <QPoint>
#include <QPushButton>

class DragTokenButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(QString token READ token WRITE setToken)

  public:
    explicit DragTokenButton(QWidget *parent = nullptr);

    QString token() const;
    void setToken(const QString &token);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

  private:
    QPoint m_pressPosition;
    QString m_token;
};
