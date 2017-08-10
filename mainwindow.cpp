#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Set up Qt toolbar window
    ui->setupUi(this);
    ui->pointTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->pointEditorPanel->hide();
    ui->frameSlider->setEnabled(false);
    ui->frameSpinBox->setEnabled(false);

    // Set window icon
    QPixmap logo = QPixmap(":/Logo/northern-red.png");
    //setWindowIcon(QIcon(logo));

    // Set up scene
    pen = QPen(QColor(50,205,50, 100), 1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin);
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    ui->insetView->setScene(scene);
    ui->insetView->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));

    // Set up frame
    image_item = new ImageItem();
    image_item->setPixmap(logo);
    scene->addItem(image_item);

    // Set up chart
    chart = new Chart;
    chart->setTitle("Dynamic spline chart");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::AllAnimations);
    chart_view = new QChartView(chart);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setFixedSize(300,400);
    ui->chartLayout->addWidget(chart_view);

    // Connect signals
    connect(image_item, SIGNAL(currentPositionRgbChanged(QPointF&)), this, SLOT(showMousePosition(QPointF&)));
    connect(image_item, SIGNAL(pixelClicked(QPointF&)), this, SLOT(onPixelClicked(QPointF&)));
    connect(ui->pointTable, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(on_pointTable_currentCellChanged(int,int,int,int)));
    show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showMousePosition(QPointF &pos)
{
    if (!current_frame.empty())
    {
        cv::Size mat_size = current_frame.size();
        int x = static_cast<int>(pos.x());
        int y = static_cast<int>(pos.y());
        if (x >= 0 && y >= 0 && x <= mat_size.width && y <= mat_size.height)
        {
            ui->mousePositionLabel->setText("x: " + QString::number(x) + ", y: " + QString::number(y));
        }
    }
}

void MainWindow::onPixelClicked(QPointF &pos)
{
    if (!current_frame.empty())
    {
        cv::Size mat_size = current_frame.size();
        int x = static_cast<int>(pos.x());
        int y = static_cast<int>(pos.y());
        if (x >= 0 && y >= 0 && x <= mat_size.width && y <= mat_size.height)
        {
            int row = ui->frameSlider->value()-1;
            QString text = QString("%1, %2").arg(x).arg(y);
            ui->pointTable->setItem(row, 0, new QTableWidgetItem(text));
            removeAllSceneEllipses();
            removeAllSceneLines();
            drawCrosshair(x, y);

            // Update inset
            ui->insetView->centerOn(x,y);
        }
    }
}

void MainWindow::removeAllSceneEllipses()
{
    foreach (QGraphicsItem *item, scene->items())
    {
        QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
        if (ellipse)
        {
            scene->removeItem(ellipse);
        }
    }
}

void MainWindow::removeAllSceneLines()
{
    foreach (QGraphicsItem *item, scene->items())
    {
        QGraphicsLineItem *line = qgraphicsitem_cast<QGraphicsLineItem *>(item);
        if (line)
        {
            scene->removeItem(line);
        }
    }
}

void MainWindow::drawCrosshair(int x, int y)
{
    scene->addEllipse( x-5, y-5, 10, 10, pen);
    scene->addLine(x-4, y, x+4, y, pen);
    scene->addLine(x, y-4, x, y+4, pen);
}

void MainWindow::savePointsToCSV(QString filename)
{
    QString text_data;
    int row_count = ui->pointTable->rowCount();
    text_data += QString("x,y,\n");
    for (int row=0; row<row_count; row++)
    {
        QTableWidgetItem* item = ui->pointTable->item(row, 0);
        if (item)
        {
            QStringList coordinate = item->text().split(",");
            text_data += coordinate[0];
            text_data += ",";
            text_data += coordinate[1];
            text_data += ",";
            text_data += "\n";
        }
    }
    QFile csv_file(filename);
    if(csv_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {

        QTextStream out(&csv_file);
        out << text_data;

        csv_file.close();
    }
    qDebug() << "saved all points: " << filename;
}

void MainWindow::on_frameSpinBox_valueChanged(int arg1)
{
    qDebug() << "frame update.";
    int frame_index = arg1-1;
    if (frame_contours.size()>0)
    {
    qDebug() << "boom: " << frame_contours.size();
    cap.set(CV_CAP_PROP_POS_FRAMES, frame_index);
    cap.read(current_frame);

    /// Draw contours
    cv::RNG rng(12345);
    ContourList contours = frame_contours.at(frame_index);
    Hierarchy hierarchy = frame_hierarchies.at(frame_index);
    /// Draw contours
    qDebug() << "contour count: " << contours.size();
//    for (int i = 0; i< 30; i++)
   // for (int i = 0; i< contours.size(); i++)
   // {
        //cv::Scalar color = contour_colors.at(i);
    cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
    cv::drawContours(current_frame, contours, 0, color, 1, 8, hierarchy, 0, cv::Point());
   // }
//    std::vector<cv::Scalar> color_set = contour_colors.at(frame_index);
//    for (int i=0; i< contours.size(); i++)
//    {
//        cv::Scalar color = color_set.at(i);
//        cv::drawContours(current_frame, contours, i, color, 1, 8, hierarchy, 0, cv::Point());
//    }

    /// Show in a window
    img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
    QPixmap pixmap;
    pixmap = QPixmap::fromImage(img);

    /// Show in view, scaled to view bounds & keeping aspect ratio
    image_item->setPixmap(pixmap);

    /// Determine where to place the ellipse based on the frame value and its associated (x,y) position
    removeAllSceneEllipses();
    removeAllSceneLines();
    QTableWidgetItem* item = ui->pointTable->item(frame_index, 0);
    if (item)
    {
        QStringList coordinate = item->text().split(",");
        int x = (coordinate[0]).toInt();
        int y = (coordinate[1]).toInt();
        drawCrosshair(x, y);

        /// Update inset
        ui->insetView->centerOn(x,y);
    }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!current_frame.empty())
    {
        img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(img);

        // Show in view, scaled to view bounds & keeping aspect ratio
        image_item->setPixmap(pixmap);
        QRectF bounds = scene->itemsBoundingRect();
        ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
        ui->graphicsView->centerOn(0,0);
    }
}

void MainWindow::on_action_Open_triggered()
{       
    /// Load a video
    QString result = QFileDialog::getOpenFileName(this, tr("Select a Video File"), "/home", tr("Video Files (*.avi)"));
    video_filepath = result.toUtf8().constData();
    QString video_filename = QString::fromStdString(video_filepath.substr(video_filepath.find_last_of("/\\") + 1));
    cap = cv::VideoCapture(video_filepath);
    qDebug() << "opened video: " << video_filename;

    /// Enable video control elements, update elements with video information.
    int frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);
    ui->pointEditorPanel->show();
    ui->pointTable->setRowCount(frame_count);
    ui->videoComboBox->addItem(video_filename);

    /// Find contours
    int thresh = 100;
    cv::RNG rng(12345);
    cv::Mat src_gray;
    cv::Mat canny_output;
    int max_size = 0;
    std::vector<std::vector<cv::Point>> frame_centroids(frame_count);
    ContourListSet initial_contours;
    HierarchyListSet initial_hierarchies;
    for (int i=0; i<frame_count; i++)
    {
        cap.set(CV_CAP_PROP_POS_FRAMES, i);
        cap.read(current_frame);

        /// Convert image to gray and blur it
        cv::cvtColor(current_frame, src_gray, CV_BGR2GRAY);
        cv::blur(src_gray, src_gray, cv::Size(3,3));

        /// Detect edges using canny
        cv::Canny(src_gray, canny_output, thresh, thresh*2, 3);

        /// Find contours
        std::vector<cv::Vec4i> hierarchy;
        ContourList contours;
        cv::findContours(canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
        initial_contours.push_back(contours);
        initial_hierarchies.push_back(hierarchy);
        if (contours.size() > max_size)
        {
            max_size = contours.size();
        }

        /// Find mean of each contour
        for(int j=0; j<contours.size(); j++)
        {
            std::vector<cv::Point> points;
            points = contours.at(j);
            cv::Point zero(0.0f, 0.0f);
            cv::Point sum  = std::accumulate(points.begin(), points.end(), zero);
            cv::Point mean_point = (sum * (1.0f / points.size()));
            //qDebug() << "center: " << mean_point.x << ", " << mean_point.y;
            frame_centroids[i].push_back(mean_point);
        }
    }
    qDebug() << "Completed finding contour centroids.";

    /// Match first contour
    ContourList fc;
    fc.push_back(initial_contours.at(0).at(0));
    frame_contours.push_back(fc);
    Hierarchy fh;
    fh.push_back(initial_hierarchies.at(0).at(0));
    frame_hierarchies.push_back(fh);
    std::vector<cv::Point> first_set = frame_centroids.at(0);
    cv::Point first_point = first_set.at(0);
    for (int i=1; i<frame_count; i++)
    {
        std::vector<cv::Point> point_set = frame_centroids.at(i);
        int best_distance = current_frame.cols;
        int index = -1;
        for (int k=0; k<point_set.size(); k++)
        {
            cv::Point point = point_set.at(k);
            double dist = cv::norm(first_point-point);

            if (dist < best_distance)
            {
                best_distance = dist;
                index = k;
            }
        }
        first_point = point_set.at(index);
        ContourList fcc;
        fcc.push_back(initial_contours.at(i).at(index));
        frame_contours.push_back(fcc);
        Hierarchy fhh;
        fhh.push_back(initial_hierarchies.at(i).at(index));
        frame_hierarchies.push_back(fhh);
    }

    /// show frame zero
    cap.set(CV_CAP_PROP_POS_FRAMES, 0);
    cap.read(current_frame);
    img = QImage((uchar*) current_frame.data, current_frame.cols, current_frame.rows, current_frame.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(img);

    /// Show in view, scaled to view bounds & keeping aspect ratio
    image_item->setPixmap(pixmap);
    QRectF bounds = scene->itemsBoundingRect();
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);
    ui->graphicsView->centerOn(0,0);

    /// 5X scale in inset view
    ui->insetView->scale(5,5);
    ui->insetView->centerOn(bounds.center());

    /// Set up chart
    chart->axisX()->setRange(1, frame_count);

    /// Enable frame sliders
    ui->frameSlider->setEnabled(true);
    ui->frameSlider->setRange(1, frame_count);
    ui->frameSpinBox->setEnabled(true);
    ui->frameSpinBox->setRange(1, frame_count);
}

void MainWindow::on_pointTable_currentCellChanged(int row, int column, int previous_row, int previous_column)
{
    // Update the frame based on the row selected
    QTableWidgetItem* item = ui->pointTable->item(row, column);
    ui->frameSpinBox->setValue(row+1);

    // Enable/disable the button for deleting points
    if (item && !item->text().trimmed().isEmpty())
    {
        ui->deletePointButton->setEnabled(true);
    }
    else
    {
        ui->deletePointButton->setEnabled(false);
    }
}

void MainWindow::on_saveButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
        tr("Save pixel positions"), "",
        tr("CSV (*.csv);;All Files (*)"));
    if (!filename.trimmed().isEmpty())
    {
        savePointsToCSV(filename);
    }
}
