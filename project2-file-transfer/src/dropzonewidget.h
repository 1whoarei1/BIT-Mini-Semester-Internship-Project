#ifndef DROPZONEWIDGET_H
#define DROPZONEWIDGET_H

#include <QLabel>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;

// 支持本地单文件拖放的标签控件，通过重写拖拽事件向主窗口报告文件路径。
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
