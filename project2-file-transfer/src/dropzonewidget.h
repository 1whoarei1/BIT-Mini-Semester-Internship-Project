#ifndef DROPZONEWIDGET_H
#define DROPZONEWIDGET_H

#include <QLabel>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;

class DropZoneWidget : public QLabel
{
    Q_OBJECT

public:
    explicit DropZoneWidget(QWidget *parent = nullptr);

signals:
    void fileDropped(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setDragActive(bool active);
};

#endif
