#pragma once

#include <QLineEdit>

class DropLineEdit : public QLineEdit
{
    Q_OBJECT

  public:
    explicit DropLineEdit(QWidget *parent = nullptr);
    static QString tokenMimeType();

  signals:
    void tokenDropped(const QString &token);

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};
