/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include "vtkPolyLine.h"
#include "wndantennadesigner.h"
#include <QtGui/QImage>
#include <QtCore/qmath.h>
#include <QFileDialog>
#include <QFileInfo>
#include "ui_wndantennadesigner.h"
#include <map>
#include <Eigen/Eigen>
//----------------------------------------------------------
// VTK
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkPlotSurface.h>
#include <vtkQuad.h>
#include <vtkDoubleArray.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>
#include <vtkSphereSource.h>
#include <vtkLabeledDataMapper.h>
#include <vtkGlyph3DMapper.h>
#include <vtkTextMapper.h>
#include <vtkGlyph3DMapper.h>


extern QDir             ariasdk_modules_path;
extern QDir             ariasdk_projects_path;
extern QDir             ariasdk_scripts_path;
extern QDir             ariasdk_antennas_path;
extern QDir             ariasdk_antennaff_path;
//-------------------------------------------------------
wndAntennaDesigner::wndAntennaDesigner(antenna* ant, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::wndAntennaDesigner),
    _points(nullptr),
    _polydata(nullptr),
    currentAntenna()
{
    de = nullptr;
    ui->setupUi(this);

    QObject::connect(ui->cbSelectVariable, &QComboBox::currentIndexChanged,     this, &wndAntennaDesigner::evalRadiatedField);
    QObject::connect(ui->btnEvalRadiatedField, &QPushButton::clicked,           this, &wndAntennaDesigner::evalRadiatedField);
    QObject::connect(ui->cbComplexField,&QComboBox::currentIndexChanged,        this, &wndAntennaDesigner::cbComplexValueSelected);
    QObject::connect(ui->cbFieldSelect, &QComboBox::currentIndexChanged,        this, &wndAntennaDesigner::cbFieldSelected);
    QObject::connect(ui->cbFreqSlider, &QSlider::valueChanged,                  this, &wndAntennaDesigner::cbFreqSliderChanged);
    QObject::connect(ui->btnImportFF, &QPushButton::clicked,                    this, &wndAntennaDesigner::importFFIO);
    QObject::connect(ui->btnLoad, &QPushButton::clicked,                        this, &wndAntennaDesigner::loadAntenna);
    QObject::connect(ui->btnSave, &QPushButton::clicked,                        this, &wndAntennaDesigner::saveAntenna);
    QObject::connect(ui->cbAvailableAntennas, &QComboBox::currentIndexChanged,  this, &wndAntennaDesigner::currentAntennaChanged);
    QObject::connect(ui->btnExportWs, &QPushButton::clicked,                    this, &wndAntennaDesigner::exportFF);

    dontUpdateGraph = false;

    _colors             = vtkNew<vtkNamedColors>();
    _actor              = vtkNew<vtkActor>();
    _points             = vtkNew<vtkPoints>();
    _polydata           = vtkNew<vtkPolyData>();
    _polydatamapper     = vtkNew<vtkPolyDataMapper>();
    _renderer           = vtkNew<vtkRenderer>();
    _cells              = vtkNew<vtkCellArray>();
    _gain               = vtkNew<vtkDoubleArray>();
    _axes               = vtkNew<vtkCubeAxesActor2D>();
    _normals            = vtkNew<vtkPolyDataNormals>();
    _outline_filter     = vtkNew<vtkOutlineFilter>();
    _color_bar          = vtkNew<vtkScalarBarActor>();
    _lookup_table       = vtkNew<vtkLookupTable>();

    // Line 3dB
    _text_mapper        = vtkNew<vtkTextMapper>();
    _text_actor         = vtkNew<vtkActor2D>();
    _line_grid          = vtkNew<vtkUnstructuredGrid>();
    _line_mapper        = vtkNew<vtkDataSetMapper>();
    _line_actor         = vtkNew<vtkActor>();
    _points_mapper      = vtkNew<vtkGlyph3DMapper>();
    _glyphs_actor       = vtkNew<vtkActor>();

    vtkNew<vtkSphereSource> sphere;
    sphere->SetPhiResolution(21);
    sphere->SetThetaResolution(21);
    sphere->SetRadius(.08);


    // Set the background color.
    std::array<unsigned char, 4> bkg{{91, 81, 81, 255}};
    _colors->SetColor("BkgColor", bkg.data());

    _axes->SetInputConnection(_normals->GetOutputPort());
    _axes->SetCamera(_renderer->GetActiveCamera());
    _axes->SetLabelFormat("%1.3g");

    _outline_filter->SetInputConnection(_normals->GetOutputPort());

    _normals->SetInputData(_polydata);
    _polydatamapper->SetInputData(_polydata);
    _polydatamapper->ScalarVisibilityOn();
    _polydatamapper->SetScalarModeToUsePointData();
    _polydatamapper->SetColorModeToMapScalars();

    // Main actor
    _actor->SetMapper(_polydatamapper);

    //-3dB line and points
    _line_mapper->SetInputData(_line_grid);
    _line_actor->SetMapper(_line_mapper);
    _line_actor->GetProperty()->EdgeVisibilityOn();
    _line_actor->GetProperty()->SetLineWidth(3);
    _line_actor->GetProperty()->SetColor(_colors->GetColor3d("Black").GetData());
    _renderer->AddActor(_line_actor);

    vtkSmartPointer<vtkLabeledDataMapper> labelMapper = vtkNew<vtkLabeledDataMapper>();
    labelMapper->SetInputData(_line_grid);

    _text_actor->SetMapper(labelMapper);

    // Glyph the points
    _points_mapper->SetInputData(_line_grid);
    _points_mapper->SetSourceConnection(sphere->GetOutputPort());
    _points_mapper->ScalingOff();
    _points_mapper->ScalarVisibilityOff();


    _glyphs_actor->SetMapper(_points_mapper);
    _glyphs_actor->GetProperty()->SetColor(
            _colors->GetColor3d("Banana").GetData());

    _glyphs_actor->GetProperty()->SetSpecular(.6);
    _glyphs_actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
    _glyphs_actor->GetProperty()->SetSpecularPower(100);

    fillAntennaData();

    _renderer->AddActor((vtkProp*)(_actor.GetPointer()));
    _renderer->AddActor2D((vtkProp*)(_axes.GetPointer()));
    _renderer->AddActor2D(_color_bar);
    _renderer->AddActor(_line_actor);
    _renderer->AddActor(_glyphs_actor);
    _renderer->AddActor(_text_actor);

    _renderer->SetBackground(_colors->GetColor3d("BkgColor").GetData());
    // Zoom in a little by accessing the camera and invoking its "Zoom" method.
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom(0.75);

    ui->plot->renderWindow()->AddRenderer(_renderer);

    if (ant!=nullptr)
    {
        currentAntenna.load(ant->get_full_filepath());
        updateWndData();
    }
}
//-------------------------------------------------------
void wndAntennaDesigner::setDataEngine(octaveInterface* dws)
{
    de = dws;
    ui->btnExportWs->setEnabled(de!=nullptr);
}
//-------------------------------------------------------
wndAntennaDesigner::~wndAntennaDesigner()
{
    delete ui;
}

//-------------------------------------------------------
//! [5]
void wndAntennaDesigner::setAxisXRange(float min, float max)
{

}
//-------------------------------------------------------
void wndAntennaDesigner::setAxisYRange(float min, float max)
{

}

//-------------------------------------------------------
void wndAntennaDesigner::setAxisZRange(float min, float max)
{

}
//! [5]
//-------------------------------------------------------
//-------------------------------------------------------
void wndAntennaDesigner::evalRadiatedField()
{

}
//-------------------------------------------------------
void wndAntennaDesigner::cbFieldSelected(int field)
{
    fillAntennaData();
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom(0.75);
    ui->plot->renderWindow()->Render();
}
//-------------------------------------------------------
void wndAntennaDesigner::cbComplexValueSelected(int field)
{
    fillAntennaData();
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom(0.75);
    ui->plot->renderWindow()->Render();

}
//-------------------------------------------------------
void wndAntennaDesigner::cbFreqSliderChanged(int val)
{
    double freq = currentAntenna.getFreqs().elem(val);
    ui->lblFreq->setText(QString("Frequency ")+QString::number(freq/1e6)+QString(" MHz"));
    if (dontUpdateGraph) return;
    fillAntennaData();
}
//-------------------------------------------------------
void wndAntennaDesigner::fillAntennaData()
{
    NDArray a = currentAntenna.getAzimuth();
    NDArray z = currentAntenna.getZenith();

    int freqid = ui->cbFreqSlider->sliderPosition();
    int field  = ui->cbFieldSelect->currentIndex();
    int cplx   = ui->cbComplexField->currentIndex();

    _points->Reset();
    _cells->Reset();
    _gain->Reset();
    _gain->SetNumberOfComponents(1);

    NDArray v;

    switch (field)
    {
    case 0:  // Theta
        switch (cplx)
        {
        case 0: // Real
            v = real(currentAntenna.getEt(freqid));
            break;
        case 1: // Imag
            v = imag(currentAntenna.getEt(freqid));
            break;
        case 2: // abs
            v = currentAntenna.getEt(freqid).abs();
            break;
        case 3: // Arg
            v = (octave_value(currentAntenna.getEt(freqid)).arg()).array_value();
            break;
        case 4: // dB
            v = dB_field(currentAntenna.getEt(freqid));
            break;
        }
        break;
    case 1: // Phi
        switch (cplx)
        {
        case 0: // Real
            v = real(currentAntenna.getEp(freqid));
            break;
        case 1: // Imag
            v = imag(currentAntenna.getEp(freqid));
            break;
        case 2: // abs
            v = currentAntenna.getEp(freqid).abs();
            break;
        case 3: // Arg
            v = (octave_value(currentAntenna.getEp(freqid)).arg()).array_value();
            break;
        case 4: // dB
            v = dB_field(currentAntenna.getEp(freqid));
            break;
        }
        break;
    case 2: // Gain
        switch (cplx)
        {
        case 0: // Real
            v = currentAntenna.getGain(freqid);
            break;
        case 1: // Imag
            v = NDArray(currentAntenna.getGain(freqid).dims(),0);
            break;
        case 2: // abs
            v = currentAntenna.getGain(freqid);
            break;
        case 3: // Arg
            v = NDArray(currentAntenna.getGain(freqid).dims(),0);
            break;
        case 4: // dB
            v = dB_field(currentAntenna.getGain(freqid),false);
            break;
        }
        break;

    }

    double minRadius = 1e-47;
    int na = a.numel();
    int nz = z.numel();
    int nv = v.numel();
    double rmin =1e99;
    double rmax =-1e99;

    for (int vi = 0; vi < nv; vi++)
    {
        double vact = v(vi);
        if (vact < rmin)
            rmin = vact;
        if (vact > rmax)
            rmax = vact;
    }


    double cAdd = 0;
    if (rmin <= minRadius)
        cAdd = -rmin + minRadius;

    double vmin = 9e99;
    double vmax = -9e99;
    double ai_max = 0;
    double zi_max = 0;
    // Go with the series points

    std::map<int,std::map<int,int>> cell_map;
    for (int zi = 0; zi < nz; zi++)
    {
        double s0 = sin(z(zi));
        double c0 = cos(z(zi));

        for (int ai=0; ai <= na ; ai++)
        {
            int aindex = ai < na? ai : 0;
            double v0 = v(aindex,zi);
            v0 += cAdd;
            double xp = v0 * s0 * cos(a(aindex));
            double yp = v0 * s0 * sin(a(aindex));
            double zp = v0 * c0;

            cell_map[zi][ai] = _points->InsertNextPoint(xp, yp, zp);
            v0 -= cAdd;

            _gain->InsertNextTuple(&v0);
            if (v0 > vmax) {vmax = v0; ai_max = a(aindex); zi_max = z(zi);}
            if (v0 < vmin) vmin = v0;
        }
    }
    // Find -3dB around peak direction
    bool bFound = false;
    if (field==2)
    {
        // Versor of maximum gain
        Eigen::Vector3d versor_z_max( sin(zi_max)*cos(ai_max) ,
                                      sin(zi_max)*sin(ai_max) ,
                                      cos(zi_max));

        Eigen::Vector3d versor_y_max( sin(zi_max+M_PI/2)*cos(ai_max) ,
                                      sin(zi_max+M_PI/2)*sin(ai_max) ,
                                      cos(zi_max+M_PI/2));
        Eigen::Vector3d versor_x_max;

        versor_x_max = versor_z_max.cross(versor_y_max);
        versor_x_max.normalize();
        versor_y_max.normalize();
        versor_z_max.normalize();
        Eigen::Matrix3d rotMatrix;
        rotMatrix.col(0) = versor_x_max;
        rotMatrix.col(1) = versor_y_max;
        rotMatrix.col(2) = versor_z_max;

        Eigen::Matrix3d rotMatrixInv = rotMatrix.inverse();

        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        int numberOfVertices = 0;

        double gain_threshold = cplx==4 ? vmax - 3 : vmax * 0.5;

        double xp, yp, zp;
        for (double azimuth = 0; azimuth <= 2.0*M_PI; azimuth+= 5*M_PI/180)
        {
                for (double zenith = 0; zenith < M_PI; zenith += 5*M_PI/180)
                {
                    Eigen::Vector3d pTest(  sin(zenith)*cos(azimuth),
                                          sin(zenith)*sin(azimuth),
                                          cos(zenith));
                    pTest = rotMatrixInv * pTest;
                    double xt = pTest[0];
                    double yt = pTest[1];
                    double zt = pTest[2];
                    double r    = sqrt(xt*xt+yt*yt+zt*zt);
                    double zen  = acos(zt/r);
                    if (zen < 0) zen+=M_PI * 2.0;
                    double az   = atan2(yt,xt);
                    if (az < 0) az += M_PI * 2.0;
/*
                    qDebug() << "Peak direction          : Theta : " << QString::number(zi_max) << " Phi: " << QString::number(ai_max);
                    qDebug() << "Curr direction(own  ref): Theta : " << QString::number(zenith) << " Phi: " << QString::number(azimuth);
                    qDebug() << "Curr direction(glob ref): Theta : " << QString::number(zen) << " Phi: " << QString::number(az);
*/
                    double g = currentAntenna.getGainInterp(az,zen,currentAntenna.getFreqs()(freqid));

                    if (cplx==4)
                        g = 10*log10(g);

                    if (g <= gain_threshold)
                    {
                        double g_eq = g + cAdd;
//                        qDebug() << "Found -3dB @ :  Theta : " << QString::number(zen) << " Phi: " << QString::number(az);
                        xp = g_eq * sin(zen)*cos(az);
                        yp = g_eq * sin(zen)*sin(az);
                        zp = g_eq * cos(zen);
                        bFound = true;
                        points->InsertNextPoint(xp,yp,zp);
                        numberOfVertices++;
                        break;
                    }
                }
                if (!bFound) break;
        }
        if (bFound)
        {
//                qDebug() << "*************** ADDING 3dB LINE **************************";
                vtkSmartPointer<vtkPolyLine> polyline = vtkSmartPointer<vtkPolyLine>::New();
                polyline->GetPointIds()->SetNumberOfIds(numberOfVertices);

                for (int i = 0; i < numberOfVertices; ++i)
                    polyline->GetPointIds()->SetId(i, i);

                _line_grid->SetPoints(points);
                _line_grid->InsertNextCell(polyline->GetCellType(), polyline->GetPointIds());
        }
    }

    if ((field!=2)||(bFound==false))
    {
        vtkSmartPointer<vtkPolyLine> polyline = vtkSmartPointer<vtkPolyLine>::New();
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        _line_grid->SetPoints(points);
        _line_grid->InsertNextCell(polyline->GetCellType(), polyline->GetPointIds());

    }

    // In the reference

    // Go with the series quads
    for (int zi = 0; zi < nz-1; zi++)
        for (int ai = 0; ai < na; ai++)
        {
            vtkNew<vtkQuad> quad;
            quad->GetPointIds()->SetId(0, cell_map[zi][ai]);
            quad->GetPointIds()->SetId(1, cell_map[zi+1][ai]);
            quad->GetPointIds()->SetId(2, cell_map[zi+1][ai+1]);
            quad->GetPointIds()->SetId(3, cell_map[zi][ai+1]);
            _cells->InsertNextCell(quad);
        }

    _polydata->SetPoints(_points);
    _polydata->SetPolys(_cells);
    _polydata->GetPointData()->SetScalars(_gain);

    _lookup_table->SetTableRange(0,1);
    _lookup_table->Build();
    _polydatamapper->SetScalarRange(vmin,vmax);

    _polydatamapper->SetLookupTable(_lookup_table);
    _color_bar->SetLookupTable(_lookup_table);

    ui->plot->renderWindow()->Render();
}

//-------------------------------------------------------
void wndAntennaDesigner::createNewAntenna()
{

}
//-------------------------------------------------------
void wndAntennaDesigner::saveAntenna()
{
    QString name = ui->lnAntennaName->text();

    if ((name.isEmpty())||(name!=currentAntenna.get_name()))
    {
        QString antennaFile = QFileDialog::getSaveFileName(this,"Save antenna model file",
                                                           ariasdk_antennas_path.absolutePath()+QDir::separator()+name+QString(".antm"),
                                                           tr("Antenna Model (*.antm);;All files (*.*)"),
                                                           nullptr,QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

        if (antennaFile.isEmpty())
            return;

        QFileInfo fi(antennaFile);
        currentAntenna.set_name(fi.fileName());
        ui->lnAntennaName->setText(currentAntenna.get_name());
        if (!currentAntenna.save(antennaFile))
            QMessageBox::critical(this,"Error","Error while saving");
        else
            ariasdk_antennas_path.setPath(fi.absolutePath());

        return;
    }

    if (!currentAntenna.save())
        QMessageBox::critical(this,"Error","Error while saving");

}
//-------------------------------------------------------
void wndAntennaDesigner::loadAntenna()
{
    QString antennaFile = QFileDialog::getOpenFileName(this,"Open antenna model file",
                                                    ariasdk_antennas_path.absolutePath()+QDir::separator(),
                                                    tr("Antenna Model (*.antm);;All files (*.*)"),
                                                    nullptr,
                                                    QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

    if (antennaFile.isEmpty()) return;
    antenna test;

    if (!test.load(antennaFile))
    {
        QMessageBox::critical(this,"Error","Error while loading antenna data");
        return;
    }
    currentAntenna = test;
    ariasdk_antennas_path.setPath(QFileInfo(antennaFile).absolutePath());
    ui->lnAntennaName->setText(currentAntenna.get_name());
    if (!ui->cbAvailableAntennas->findText(currentAntenna.get_name()))
        ui->cbAvailableAntennas->addItem(currentAntenna.get_name());

    updateWndData();
}
//-------------------------------------------------------
void wndAntennaDesigner::bringToProject()
{

}

void wndAntennaDesigner::updateWndData()
{
    NDArray freqs = currentAntenna.getFreqs();
    ui->cbFreqSlider->setMinimum(0);
    ui->cbFreqSlider->setMaximum(freqs.numel()-1);
    ui->cbFreqSlider->setTickInterval(1);
    ui->cbFreqSlider->setValue(0);
    ui->lblFreq->setText(QString("Frequency ")+QString::number(freqs(0)/1e6)+QString(" MHz"));
    dontUpdateGraph = false;

    fillAntennaData();

    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom(0.75);
    ui->plot->renderWindow()->Render();

}
//-------------------------------------------------------
void wndAntennaDesigner::importFFIO()
{
    QStringList ffioFile = QFileDialog::getOpenFileNames(this,"Open farfield file",
                                                            ariasdk_antennaff_path.absolutePath()+QDir::separator(),
                                                            tr("FFIO (*.ffio);;All files (*.*)"),
                                                            nullptr,
                                                            QFileDialog::Options(QFileDialog::Detail|QFileDialog::DontUseNativeDialog));

    if (ffioFile.isEmpty()) return;
    ariasdk_antennaff_path.setPath(QFileInfo(ffioFile[0]).absolutePath());
    currentAntenna.loadFarField(ffioFile);
    dontUpdateGraph = true;
    updateWndData();
}

void wndAntennaDesigner::exportFF()
{
    if (de==nullptr)
        return;

    QString name = currentAntenna.get_name();
    QFileInfo fi(name);
    name = fi.baseName();
    // Put everything into a struct
    octave_map antenna_struct;

    octave_value a(currentAntenna.getAzimuth());
    antenna_struct.assign("azimuth",a);

    octave_value z(currentAntenna.getZenith());
    antenna_struct.assign("zenith",z);

    octave_value f(currentAntenna.getFreqs());
    antenna_struct.assign("freq",f);
    octave_value ep = currentAntenna.getEp();
    antenna_struct.assign("ep", ep);
    octave_value et = currentAntenna.getEt();
    antenna_struct.assign("et", et);

    de->get_workspace()->add_variable(name.toStdString(),false, antenna_struct);
    emit de->updatedVariable(name.toStdString());
/*

    QString name = currentAntenna.get_name();
    QFileInfo fi(name);
    name = fi.fileName();

    octave_value a(currentAntenna.getAzimuth());
    QString azimuth_name(name+QString("_azimuth"));
    azimuth_name=de->appendVariable(azimuth_name,a,true);

    QString zenith_name(name+QString("_zenith"));
    octave_value z(currentAntenna.getZenith());
    zenith_name= de->appendVariable(zenith_name,z,true);

    int freqid = ui->cbFreqSlider->sliderPosition();
    int field  = ui->cbFieldSelect->currentIndex();
    int cplx   = ui->cbComplexField->currentIndex();

    octave_value fieldval;
    QString fieldName = name;
    switch (field)
    {
    case 0:  // Theta
        fieldval = octave_value(currentAntenna.getEt(freqid));
        fieldName+="_ETheta";
        break;
    case 1: // Phi
        fieldval = octave_value(currentAntenna.getEp(freqid));
        fieldName+="_EPhi";
        break;
    case 2: // Gain
        switch (cplx)
        {
        case 4: // dB
            fieldval = octave_value(dB_field(currentAntenna.getGain(freqid),false));
            fieldName+="_Gain_dB";
            break;
        default:
            fieldval = octave_value((currentAntenna.getGain(freqid)));
            fieldName+="_Gain";
            break;
        }
        break;
    }
    ComplexNDArray fv = fieldval.complex_array_value();
    QStringList indep;

    indep.append(azimuth_name);
    indep.append(zenith_name);
    de->get_workspace()->add_variable(fieldName.toStdString(),false, fieldval);
    emit de->updatedVariable(fieldName.toStdString());
*/
}

void wndAntennaDesigner::currentAntennaChanged(int index)
{
    ui->lnAntennaName->setReadOnly(index>0);
}
