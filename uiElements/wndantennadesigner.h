/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#ifndef WNDANTENNADESIGNER_H
#define WNDANTENNADESIGNER_H

#include <QDialog>
#include "antenna.h"

#include <QtWidgets/QSlider>
#include "octaveinterface.h"

#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkNamedColors.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkDoubleArray.h>
#include <vtkPolyDataNormals.h>
#include <vtkOutlineFilter.h>
#include <vtkScalarBarActor.h>
#include <vtkLookupTable.h>
#include <vtkPointSource.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkGlyph3DMapper.h>


namespace Ui {
class wndAntennaDesigner;
}


class wndAntennaDesigner : public QDialog
{
    Q_OBJECT

public:
    explicit wndAntennaDesigner(antenna *ant, QWidget *parent = nullptr);
    ~wndAntennaDesigner();

    void enableSqrtSinModel(bool enable);
    //! [0]

    void setDataEngine(octaveInterface* dws=nullptr);

private:
    bool dontUpdateGraph;

    octaveInterface            *de;
    Ui::wndAntennaDesigner *ui;

    vtkSmartPointer<vtkPoints>              _points;
    vtkSmartPointer<vtkPolyData>            _polydata;
    vtkSmartPointer<vtkCellArray>           _cells;
    vtkSmartPointer<vtkPolyDataMapper>      _polydatamapper;
    vtkSmartPointer<vtkNamedColors>         _colors;
    vtkSmartPointer<vtkActor>               _actor;
    vtkSmartPointer<vtkRenderer>            _renderer;
    vtkSmartPointer<vtkCubeAxesActor2D>     _axes;
    vtkSmartPointer<vtkDoubleArray>         _gain;
    vtkSmartPointer<vtkPolyDataNormals>     _normals;
    vtkSmartPointer<vtkOutlineFilter>       _outline_filter;
    vtkSmartPointer<vtkScalarBarActor>      _color_bar;
    vtkSmartPointer<vtkLookupTable>         _lookup_table;
    vtkSmartPointer<vtkTextMapper>          _text_mapper;
    vtkSmartPointer<vtkActor2D>             _text_actor;
    vtkSmartPointer<vtkUnstructuredGrid>    _line_grid;
    vtkSmartPointer<vtkDataSetMapper>       _line_mapper;
    vtkSmartPointer<vtkActor>               _line_actor;
    vtkSmartPointer<vtkGlyph3DMapper>       _points_mapper;
    vtkSmartPointer<vtkActor>               _glyphs_actor;

//--------------------------------------
//  Custom data
    antenna                 currentAntenna;
    void                    setDefaultAntenna();
    void                    importAntenna();
//----------------------------------------

    float m_rangeMinX;
    float m_rangeMinZ;
    float m_stepX;
    float m_stepZ;
    int m_heightMapWidth;
    int m_heightMapHeight;

    void initGraph();
    void setAxisXRange(float min, float max);
    void setAxisYRange(float min, float max);
    void setAxisZRange(float min, float max);
    void fillSqrtSinProxy();
    void fillAntennaData();
    void updateWndData();

public slots:

    void evalRadiatedField();
    void cbFieldSelected(int field);
    void cbComplexValueSelected(int field);
    void cbFreqSliderChanged(int val);

// File mgmt
    void createNewAntenna();
    void saveAntenna();
    void loadAntenna();
    void bringToProject();
    void importFFIO();
    void currentAntennaChanged(int index);
    void exportFF();


};

#endif // WNDANTENNADESIGNER_H
