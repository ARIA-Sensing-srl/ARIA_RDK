/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */
#include "wnddirectionfocusing.h"
#include "ui_wnddirectionfocusing.h"
#include "vtkPolyLine.h"
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

#include <QDebug>

#define NUMBER_FREQS 25
#define NUMBER_AZIMUTH 100
#define NUMBER_ZENITH 50

wndDirectionFocusing::wndDirectionFocusing(QWidget *parent,radarModule* radar_module_ptr) :
    QDialog(parent),
    ui(new Ui::wndDirectionFocusing),
    _radar_module(radar_module_ptr)
{
    ui->setupUi(this);

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

    fill_antenna_data();

    _renderer->AddActor(_actor);
    _renderer->AddActor2D(_axes);
    _renderer->AddActor2D(_color_bar);
    _renderer->AddActor(_line_actor);
    _renderer->AddActor(_glyphs_actor);
    _renderer->AddActor(_text_actor);

    _renderer->SetBackground(_colors->GetColor3d("BkgColor").GetData());
    // Zoom in a little by accessing the camera and invoking its "Zoom" method.
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom(0.75);

    ui->plot_antenna_diagram->renderWindow()->AddRenderer(_renderer);
    ui->sldAzimuth->setTracking(true);
    ui->sldZenith->setTracking(true);
    ui->sldFrequency->setTracking(true);
    connect(ui->sldAzimuth, SIGNAL(valueChanged(int)), this,SLOT(slide_azimuth(int)));
    connect(ui->sldZenith, SIGNAL(valueChanged(int)), this,SLOT(slide_zenith(int)));
    connect(ui->sldFrequency, SIGNAL(valueChanged(int)), this,SLOT(slide_freq(int)));

    if (_radar_module!=nullptr)
    {
        double fmin =9e99;
        double fmax =-9e99;
        for (int na=0; na < _radar_module->get_antenna_count(); na++)
        {
            antenna_pointer ant = _radar_module->get_antenna(na);
            if (ant!=nullptr)
            {
                double f0 = ant->get_antenna_model()->getFreqMin();
                double f1 = ant->get_antenna_model()->getFreqMax();
                if (f0 < fmin) fmin = f0;
                if (f0 > fmax) fmax = f1;
            }
        }
        int nf    = NUMBER_FREQS; int naz = NUMBER_AZIMUTH; int nzen = NUMBER_ZENITH;
        double df = (fmax-fmin)/(nf-1);
        double da = 2*M_PI/(naz-1);
        double dz = M_PI/(nzen-1);
        NDArray freqs(dim_vector(1,nf));
        NDArray az(dim_vector(1,naz));
        NDArray zen(dim_vector(1,nzen));

        for (int n=0; n < nf; n++)
            freqs(n) = fmin + df * (n);

        for (int n=0; n < naz; n++)
            az(n) = da * (n);

        for (int n=0; n < nzen; n++)
            zen(n) = dz * (n);

        _radar_module->set_support(freqs,az,zen);

    }
    init_sliders();
    update_graph();
}
//----------------------------------------------------------
wndDirectionFocusing::~wndDirectionFocusing()
{
    delete ui;
}

//----------------------------------------------------------
void wndDirectionFocusing::fill_antenna_data()
{

}

//----------------------------------------------------------
void wndDirectionFocusing::update_graph()
{
    if (_radar_module == nullptr)
        return;

    _points->Reset();
    _cells->Reset();
    _gain->Reset();
    _gain->SetNumberOfComponents(1);

    int nf = ui->sldFrequency->value();
    int na = ui->sldAzimuth->value();
    int nz = ui->sldZenith->value();

    double a_ref = _radar_module->azimuth_equivalent()(na);
    double z_ref = _radar_module->zenith_equivalent()(nz);
    double f_ref = _radar_module->get_freq()(nf);
    ui->lblZen->setText(tr("Zenith")+":"+QString::number(z_ref*180.0/M_PI,'e',3));
    ui->lblAz->setText(tr("Azimiuth")+":"+QString::number(a_ref*180.0/M_PI,'e',3));
    ui->lblFreq->setText(tr("Freq")+":"+QString::number(f_ref/1e9,'e',3)+"GHz");


    _radar_module->calculate_direction_focusing(a_ref, z_ref, nf,true);

    NDArray v = _radar_module->gain_equivalent();
    NDArray a = _radar_module->azimuth_equivalent();
    NDArray f = _radar_module->freqs_equivalent();
    NDArray z = _radar_module->zenith_equivalent();

    double minRadius = 1e-47;
    na = a.numel();
    nz = z.numel();
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
    // Go with the series points
#ifdef DEBUG_FOI_FULL
    int ndebug=0;
#endif

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
#ifdef DEBUG_FOI_FULL
            if (ndebug < 10)
                qDebug() << QString::number(v0);
            ndebug++;
#endif

            double xp = v0 * s0 * cos(a(aindex));
            double yp = v0 * s0 * sin(a(aindex));
            double zp = v0 * c0;

            cell_map[zi][ai] = _points->InsertNextPoint(xp, yp, zp);
            v0 -= cAdd;

            _gain->InsertNextTuple(&v0);
            if (v0 > vmax) {vmax = v0;}
            if (v0 < vmin) vmin = v0;
        }
    }
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
/*
    vtkSmartPointer<vtkPolyLine> polyline = vtkSmartPointer<vtkPolyLine>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    _line_grid->SetPoints(points);
    _line_grid->InsertNextCell(polyline->GetCellType(), polyline->GetPointIds());
*/

    _polydata->SetPoints(_points);
    _polydata->SetPolys(_cells);
    _polydata->GetPointData()->SetScalars(_gain);

    _lookup_table->SetTableRange(0,1);
    _lookup_table->Build();
    _polydatamapper->SetScalarRange(vmin,vmax);

    _polydatamapper->SetLookupTable(_lookup_table);
    _color_bar->SetLookupTable(_lookup_table);

    ui->plot_antenna_diagram->renderWindow()->Render();

}

//----------------------------------------------------------
void wndDirectionFocusing::init_sliders()
{
    double fmin = 5e9;
    double fmax = 10e9;
    double azmin = 0;
    double azmax = 360;
    double zenmin= 0;
    double zenmax= 180;
    int    nf = NUMBER_FREQS;
    int    na = NUMBER_AZIMUTH;
    int    nz = NUMBER_ZENITH;

    if (_radar_module!=nullptr)
    {
        const NDArray& freqs = _radar_module->get_freq();
        fmin = freqs.min()(0);
        fmax = freqs.max()(0);
        nf = freqs.numel();
        const NDArray& az = _radar_module->get_azimuth();
        azmin = az.min()(0);
        azmax = az.max()(0);
        na = az.numel();
        const NDArray& zen =_radar_module->get_zenith();
        zenmin = zen.min()(0);
        zenmax = zen.max()(0);
        nz = zen.numel();
    }
    double k = 180/(M_PI);

    ui->lblAzMin->setText(QString::number(azmin*k)+"°");
    ui->lblAzMax->setText(QString::number(azmax*k)+"°");
    ui->lblZenMin->setText(QString::number(zenmin*k)+"°");
    ui->lblZenMax->setText(QString::number(zenmax*k)+"°");
    ui->lblFreqMin->setText(QString::number(fmin/1e9)+"GHz");
    ui->lblFreqMax->setText(QString::number(fmax/1e9)+"GHz");

    ui->sldFrequency->setTickInterval(1);
    ui->sldFrequency->setMinimum(0);
    ui->sldFrequency->setMaximum(nf);

    ui->sldAzimuth->setTickInterval(1);
    ui->sldAzimuth->setMinimum(0);
    ui->sldAzimuth->setMaximum(na);

    ui->sldZenith->setTickInterval(1);
    ui->sldZenith->setMinimum(0);
    ui->sldZenith->setMaximum(nz);
}


//----------------------------------------------------------
void wndDirectionFocusing::slide_azimuth(int tick)
{
    update_graph();
}

//----------------------------------------------------------
void wndDirectionFocusing::slide_zenith(int tick)
{
    update_graph();
}
//----------------------------------------------------------
void wndDirectionFocusing::slide_freq(int tick)
{
    update_graph();
}

