#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QString>
#include <QDebug>
#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Set up Qt toolbar window
    ui->setupUi(this);
    //connect(ui->frameDisplay, SIGNAL(sendMousePosition(QPoint&)), this, SLOT(showMousePosition(QPoint&)));

    // Set up scene
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
    image_item = new ImageItem();
    scene->addItem(image_item);
    connect(image_item, SIGNAL(currentPositionRgbChanged(QPointF&)), this, SLOT(showMousePosition(QPointF&)));

    show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mouse_callback(int event, int x, int y, int flags, void* userdata)
{
    MainWindow* main_window = reinterpret_cast<MainWindow*>(userdata);
    main_window->do_mouse(event, x, y);
}

void MainWindow::do_mouse(int event, int x, int y)
{
    if  ( event == cv::EVENT_LBUTTONDOWN )
    {
        qDebug() << "Left button of the mouse is clicked - position (" << x << ", " << y << ")";
    }
    else if  ( event == cv::EVENT_RBUTTONDOWN )
    {
        qDebug() << "Right button of the mouse is clicked - position (" << x << ", " << y << ")";
    }
    else if  ( event == cv::EVENT_MBUTTONDOWN )
    {
        qDebug() << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")";
    }
    else if ( event == cv::EVENT_MOUSEMOVE )
    {
        qDebug() << "Mouse move over the window - position (" << x << ", " << y << ")";
    }
}

void MainWindow::showMousePosition(QPointF &pos)
{
    qDebug() << "PIX move over the window - position (" << QString::number(pos.x()) << ", " << QString::number(pos.y()) << ")";
    if (!current_frame.empty())
    {
        cv::Size mat_size = current_frame.size();
        int scaled_x = (float)pos.x() * ((float)mat_size.width / (float)(ui->frameDisplay->pixmap()->width()));
        int scaled_y = (float)pos.y() * ((float)mat_size.height / ((float)ui->frameDisplay->pixmap()->height()));
        ui->mousePositionLabel->setText("x: " + QString::number(pos.x()) + ", y: " + QString::number(pos.y()) + ", scaled-> " +  QString::number(scaled_x) + ", y: " + QString::number(scaled_y));
    }
}

void MainWindow::on_frameSpinBox_valueChanged(int arg1)
{
    cap.set(CV_CAP_PROP_POS_FRAMES, arg1);
    cap.read(current_frame);
    img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
    pixel = QPixmap::fromImage(img);
    ui->frameDisplay->setPixmap(pixel);

    // Show in view, scaled to view bounds & keeping aspect ratio
    image_item->setPixmap(pixel);
    QRectF bounds = scene->itemsBoundingRect();
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
    ui->graphicsView->centerOn(0,0);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!current_frame.empty())
    {
        img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
        pixel = QPixmap::fromImage(img);
        int w = ui->frameDisplay->width();
        int h = ui->frameDisplay->height();
        ui->frameDisplay->setPixmap(pixel.scaled(w, h, Qt::KeepAspectRatio));

        // Show in view, scaled to view bounds & keeping aspect ratio
        image_item->setPixmap(pixel);
        QRectF bounds = scene->itemsBoundingRect();
        ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
        ui->graphicsView->centerOn(0,0);
    }
}

void MainWindow::on_action_Open_triggered()
{       
    // load a video
    QString result;
    result = QFileDialog::getOpenFileName(this, tr("Open Video File 2"), "/home", tr("Video Files (*.avi)"));
    video_filepath = result.toUtf8().constData();
    cap = cv::VideoCapture(video_filepath);
    frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);

    // update ui elements
    ui->frameSlider->setRange(0, frame_count-1);
    ui->frameSpinBox->setRange(0, frame_count-1);

    // show frame zero
    cap.set(CV_CAP_PROP_POS_FRAMES, 0);
    cap.read(current_frame);
    img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
    pixel = QPixmap::fromImage(img);
    int w = ui->frameDisplay->width();
    int h = ui->frameDisplay->height();
    ui->frameDisplay->setPixmap(pixel.scaled(w, h, Qt::KeepAspectRatio));

    // Show in view, scaled to view bounds & keeping aspect ratio
    image_item->setPixmap(pixel);
    QRectF bounds = scene->itemsBoundingRect();
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
    ui->graphicsView->centerOn(0,0);
}
