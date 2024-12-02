/*
 *  ARIA Sensing 2024
 *  Author: Cacciatori Alessio
 *  This program is licensed under LGPLv3.0
 */

#include "radarproject.h"
#include "radarmodule.h"
#include "octavescript.h"
#include "radarinstance.h"
#include "../scheduler/opscheduler.h"
#include "octavews.h"
//----------------------------------------------
//----------------------------------------------
// Project Item class
//----------------------------------------------
//----------------------------------------------
QString cstr_radar_devices("Radar devices");
QString cstr_scripts("Scripts");
QString cstr_handler("Handler");
QString cstr_radar_modules("Radar Modules");
QString cstr_antenna_models("Antenna Models");
QString cstr_comm_protocol("Protocols");

QString projectItem_Descriptor[9] = {"root","folder","radardevice","radarmodule","script","antenna","scheduler","protocol","other"};

projectItem*     projectItem::create_new_item(projectItem* parent, projectItem* leaf)
{    
    if (leaf==nullptr)
        leaf = new projectItem(parent);

    if (parent!=nullptr)
        parent->add_child(leaf);

    return leaf;
}

void    radarProject::set_workspace(octavews* ws)
{
    _workspace = ws;
    if (_workspace == nullptr) return;
    octaveInterface* interface = _workspace->data_interface();
    if (interface == nullptr) return;
    //connect(interface, SIGNAL(updatedVariable(const std::string& )), this, SLOT(variable_updated(const std::string& )));

    connect(interface, &octaveInterface::updatedVariables, this, &radarProject::variables_updated);

    QVector<radarInstance*> devices = get_available_radars();
    for (auto& device : devices)
        if (device !=nullptr)
            device->attach_to_workspace(ws);
}
//---------------------------------------
QVector<radarInstance*> radarProject::get_available_radars()
{
    QVector<radarInstance*> out;

    QVector<projectItem*> items = get_all_children(DT_RADARDEVICE);

    out.resize(items.count());
    int n=0;
    for (const auto& item:items)
        out[n++] = (radarInstance*)(item);

    return out;
}
//---------------------------------------
QVector<antenna*> radarProject::get_available_antennas()
{
    QVector<antenna*> out;

    QVector<projectItem*> items = get_all_children(DT_ANTENNA);

    out.resize(items.count());
    int n=0;
    for (const auto& item:items)
        out[n++] = (antenna*)(item);

    return out;
}
//---------------------------------------
QVector<octaveScript*> radarProject::get_available_scripts()
{
    QVector<octaveScript*> out;

    QVector<projectItem*> items = get_all_children(DT_SCRIPT);

    out.resize(items.count());
    int n=0;
    for (const auto& item:items)
        out[n++] = (octaveScript*)(item);

    return out;
}
//---------------------------------------
QVector<radarModule*> radarProject::get_available_modules()
{
    QVector<radarModule*> out;

    QVector<projectItem*> items = get_all_children(DT_RADARMODULE);

    out.resize(items.count());
    int n=0;
    for (const auto& item:items)
        out[n++] = (radarModule*)(item);

    return out;
}
//---------------------------------------
QVector<opScheduler*>   radarProject::get_available_scheduler()
{
    QVector<opScheduler*> out;

    QVector<projectItem*> items = get_all_children(DT_SCHEDULER);

    out.resize(items.count());
    int n=0;
    for (const auto& item:items)
        out[n++] = (opScheduler*)(item);

    return out;
}
//---------------------------------------
projectItem*     radarProject::load_radar_item(QDomElement& xml_node, projectItem* parent, projectItem* leaf)
{
    QVector<radarModule*> modules = get_available_modules();
    if (modules.count()==0) return nullptr;

    for (; !xml_node.isNull();xml_node = xml_node.nextSiblingElement("leaf"))
    {
        QDomElement child = xml_node.firstChildElement("leaf");
        if (!child.isNull())
        {
            load_radar_item(child,parent,leaf);
            continue;
        }
        QString type = xml_node.attribute("type","");
        QString name = xml_node.attribute("name");
        QString filename= xml_node.attribute("filename");

        if (type.isEmpty()) continue;
        if (name.isEmpty()) continue;

        //if (filename.isEmpty()) continue;

        //filename is the absolute (original) filename
        //Current basepath is taken from _radar_project_file

        // New file name
        if ((!type.isEmpty())&&(!name.isEmpty())&&(!filename.isEmpty()))
        {
            if (type != projectItem_Descriptor[DT_RADARDEVICE])
                continue;

            if (type == projectItem_Descriptor[DT_RADARDEVICE])
            {
                // find the proper radar module
                radarModule* module = nullptr;
                for (auto& mod : modules)
                    if (mod->get_name() == name)
                    {
                        module = mod;
                        break;
                    }

                if (module==nullptr) continue;

                 radarInstance* new_radar = new radarInstance(module);
                if (new_radar==nullptr)
                    return nullptr;

                new_radar->attach_to_workspace(_workspace);

                new_radar->load_extra_project_item_data(xml_node);
                add_radar_instance(new_radar);
            }

            if (type == projectItem_Descriptor[DT_ANTENNA])
            {
                QString antenna_file = leaf->get_full_filepath();
                add_antenna(antenna_file);
            }
        }
    }

    return nullptr;
}
//---------------------------------------
projectItem*     radarProject:: load_item(QDomElement& xml_node, projectItem* parent, projectItem* leaf, dataType datatype)
{
    projectItem * newparent = this;
    for (; !xml_node.isNull();xml_node = xml_node.nextSiblingElement("leaf"))
    {
        QString type        = xml_node.attribute("type","");
        QString name      = xml_node.attribute("name");
        QString filename             = xml_node.attribute("filename");

        if (type.isEmpty()) continue;
        if (name.isEmpty()) continue;

        //Current basepath is taken from *actual* _radar_project_file

        QDir current_base_path(QFileInfo(_radar_project_file).absolutePath());

        // If we are here, we are on a node whose type is exactly what we want to have
        if ((type == projectItem_Descriptor[DT_ROOT])&&(type==projectItem_Descriptor[datatype]))
        {
            // Update the filename according to the current_base_path
            _filename = QFileInfo(QDir(current_base_path),QFileInfo(filename).fileName()).absoluteFilePath();
        }

        if (type == projectItem_Descriptor[DT_FOLDER])
        {
            // If folder, build the new node
            newparent = datatype == DT_FOLDER? projectItem::create_new_item(parent, new folder(name)) : parent->get_folder(name);
        }
        if ((datatype!=DT_FOLDER)&&(datatype!=DT_ROOT))
        {
            // Radar devices and antennas are postponed since we need to build the module first
            QFileInfo fi(QDir(parent->get_full_path()),QFileInfo(filename).fileName());
            QString _new_file_name = fi.absoluteFilePath();
            if ((type == projectItem_Descriptor[DT_RADARMODULE])&&(type==projectItem_Descriptor[datatype]))
                add_radar_module(_new_file_name,parent);

            if ((type == projectItem_Descriptor[DT_SCRIPT])&&(type==projectItem_Descriptor[datatype]))
                add_script(_new_file_name,parent);

            if ((type == projectItem_Descriptor[DT_ANTENNA])&&(type==projectItem_Descriptor[datatype]))
                add_antenna(_new_file_name);

            if ((type == projectItem_Descriptor[DT_RADARDEVICE])&&(type==projectItem_Descriptor[datatype]))
                add_radar_instance(_new_file_name);

            if ((type == projectItem_Descriptor[DT_SCHEDULER])&&(type==projectItem_Descriptor[datatype]))
                add_scheduler(_new_file_name);
        }

        QDomElement sub_leaf = xml_node.firstChildElement("leaf");

        if (!sub_leaf.isNull())
            load_item(sub_leaf,newparent,leaf,datatype);
    }

    return nullptr;
}
//----------------------------------------------
projectItem::projectItem(QString name, dataType type, projectItem* parent) : _item_type(type), _name(name), itemParent(parent), _temp_root(nullptr), _filename("")
{
    if (parent!=nullptr)
        parent->add_child(this);
}
//----------------------------------------------
projectItem::~projectItem()
{
    if (itemParent!=nullptr)
        itemParent->qlChild.removeAll(this);

    while (qlChild.count()!=0)
    {
        projectItem *item = qlChild.last();
        delete item;
        qlChild.removeLast();
    }
}
//----------------------------------------------
// Simply update the base_dir reference
void    projectItem::move_to_new_basedir(QString new_basedir, QString relative_dir,  bool keep_original_destination , bool copy )
{
  if (itemParent==nullptr)
  {
      _filename = QFileInfo(QDir(get_full_path()), QFileInfo(_filename).fileName()).absoluteFilePath();
      return;
  }
  itemParent->move_to_new_basedir(new_basedir, relative_dir, keep_original_destination, copy);
}
//----------------------------------------------
QString projectItem::get_item_relative_path_from(QString folder)
{
    return QDir(folder).relativeFilePath(get_full_filepath());
}
//----------------------------------------------
// Return the absolute path.
// If _base_dir is empty, we just return the _filename path (regardless of it being relative or absolute)
// If _filename is absolute, we just return its path (this combo should never happen but just in case)
// Otherwise return the _base_dir+_filename path
QString projectItem::get_full_path()
{
    if (_item_type == DT_FOLDER)
    {
        if (itemParent == nullptr)
            return QFileInfo(_filename).absolutePath() + QDir::separator();
        else
            return itemParent->get_full_path() + _filename;
    }
    else return itemParent == nullptr ? QFileInfo(_filename).absolutePath()+QDir::separator() : itemParent->get_full_path();
}
//----------------------------------------------
// Return the absolute file+path.
// If _base_dir is empty, we just return the _filename (regardless of it being relative or absolute)
// If _filename is absolute, we just return it (this combo should never happen but just in case)
// Otherwise return the _base_dir+_filename
QString projectItem::get_full_filepath()
{
    if ((itemParent==nullptr)&&(_item_type!=DT_FOLDER))
        return _filename;
    if (_item_type==DT_FOLDER)
        return get_full_path();
    else
        return QFileInfo(QDir(itemParent->get_full_path()), _filename).absoluteFilePath();
}
//----------------------------------------------
void projectItem::save_xml(QDomDocument& doc, QDomElement& root)
{
    QDomElement project_el = doc.createElement("leaf");

    project_el.setAttribute("type",projectItem_Descriptor[int(_item_type)]);
    project_el.setAttribute("name", _name);
    project_el.setAttribute("filename",QFileInfo(_filename).fileName());
/*
    if (_item_type==DT_RADARDEVICE)
    {
        ((radarInstance*)(this))->save_xml();
    }

    if (_item_type==DT_SCHEDULER)
    {
        ((opScheduler*)(this))->save_xml();
    }
*/
    const QList<projectItem*>& qlx = qlChild;
    for (auto child:qlx)
    {
        child->save_xml(doc,project_el);
    }
    root.appendChild(project_el);
}

//----------------------------------------------
projectItem::projectItem(projectItem* parent) : _item_type(DT_OTHER), _name("newProjectItem")
{
    _temp_root = nullptr;
    itemParent = parent;
    if (itemParent==nullptr)
        _item_type = DT_ROOT;

    if (_item_type==DT_ROOT)
        itemParent = nullptr;
    // if we have a root, define by default 4 subsets
    if (itemParent!=nullptr)
        itemParent->qlChild.append(this);


}
//----------------------------------------------
int  projectItem::child_count()
{
    return qlChild.count();
}
//----------------------------------------------
projectItem*    projectItem::get_parent()
{
    return itemParent;
}
//----------------------------------------------
radarProject*        projectItem::get_root()
{
    if (_temp_root != nullptr)
        return _temp_root;
    if (itemParent!=nullptr)
        return itemParent->get_root();
    return (radarProject*)this;
}
//----------------------------------------------
dataType        projectItem::get_type()
{
    return _item_type;
}
//----------------------------------------------
void    projectItem::add_child(projectItem *data)
{
    if (data==nullptr) return;
    if (qlChild.contains(data)) return;
    qlChild.append(data);
    data->itemParent = this;
}

//----------------------------------------------
void    projectItem::remove_all_children()
{
    const QList<projectItem*>& qlc = qlChild;
    for (auto child: qlc)
    {
        child->remove_all_children();
    }

    for (auto child: qlc)
    {
        if (child!=nullptr)
            delete child;
    }

    qlChild.clear();
}
//----------------------------------------------
void    projectItem::remove_child(projectItem* item_to_remove)
{

    item_to_remove->remove_all_children();
    qlChild.removeAll(item_to_remove);
}
//----------------------------------------------
const projectItem&  projectItem::operator = (const projectItem& item)
{
    // Don't overwrite type, since we call inherited "=" operator from radar_instance
    if (!((_item_type == DT_RADARDEVICE)&&(item._item_type==DT_RADARMODULE)))
        _item_type = item._item_type;
    _name      = item._name;
    itemParent = item.itemParent;
    qlChild    = item.qlChild;
    _filename  = item._filename;
    _temp_root = item._temp_root;
    return *this;
}
//----------------------------------------------
projectItem* projectItem::child_item(int n)
{
    if ((n<0)||(n>=qlChild.count())) return nullptr;
    return qlChild.at(n);
}

//----------------------------------------------
//----------------------------------------------
//Find the first folder child with given name
//----------------------------------------------
//----------------------------------------------
projectItem*    projectItem::get_folder(QString folder_name)
{
    for (int n=0 ; n < qlChild.count(); n++)
        if (qlChild[n]!=nullptr)
        {
            if (qlChild[n]->_item_type!=DT_FOLDER)
                continue;

            if (qlChild[n]->get_name()==folder_name)
                return qlChild[n];
        }

    for (int n=0; n < qlChild.count(); n++)
    {
        projectItem* out = qlChild[n]->get_folder(folder_name);
        if (out!=nullptr) return out;
    }
    return nullptr;
}
//----------------------------------------------
projectItem*    projectItem::get_parent(projectItem* item)
{
    for (int n=0 ; n < qlChild.count(); n++)
        if (qlChild[n]==item)
            return this;

    for (int n=0 ; n < qlChild.count(); n++)
    {
        projectItem* out = qlChild[n]->get_parent(item);
        if (out!=nullptr) return out;
    }

    return nullptr;
}

//----------------------------------------------
projectItem*    projectItem::get_child(QString name, int type) // Use -1 to search for "any"
{
    for (int n=0 ; n < qlChild.count(); n++)
        if (qlChild[n]->get_name()==name)
        {
            if ((type==qlChild[n]->_item_type)||(type==-1))
                return qlChild[n];
        }
    for (int n=0 ; n < qlChild.count(); n++)
    {
        projectItem* out = qlChild[n]->get_child(name,type);
        if (out!=nullptr) return out;
    }

    return nullptr;
}
//----------------------------------------------
QString projectItem::get_item_descriptor()
{
    switch(_item_type)
    {
    case DT_RADARDEVICE:
    {
        return ((radarInstance*)(this))->get_item_descriptor();
    }
    default:
        return _name;
    }
    return QString("unknown");

}
//----------------------------------------------
QVector<projectItem*>   projectItem::get_all_children(int type)
{
    QVector<projectItem*> out;
    const QList<projectItem*>& children = qlChild;
    for (const auto& child: children)
    {
        if ((type==-1)||(type==child->_item_type))
            out.append(child);
    }
    for (const auto& child: children)
        out.append(child->get_all_children(type));

    return out;
}
//----------------------------------------------
void    projectItem::set_temporary_project(radarProject* project)
{
    _temp_root = project;
}
//----------------------------------------------
void    projectItem::remove_temporary_project()
{
    _temp_root = nullptr;
}
//----------------------------------------------
bool    projectItem::has_child(projectItem* item)
{
    return qlChild.contains(item);
}
//----------------------------------------------
//----------------------------------------------
//Project folders
//----------------------------------------------
//----------------------------------------------
folder::folder(QString foldername, QString basedir): projectItem(foldername, DT_FOLDER)
 {
    _filename="";

    if (!basedir.isEmpty())
    {
        QDir dir(basedir);
        if (!(QDir(QDir(basedir).cleanPath(foldername)).exists()))
            dir.mkdir(foldername);
    }
    _filename = foldername+QDir::separator();

    set_name(foldername);
 };
//----------------------------------------------
//----------------------------------------------
//Project Tree Handler
//----------------------------------------------
//----------------------------------------------
// We assume that subpath of projectName does not exists or,
// if it exists, it is empty
 // If not, create an empty project
radarProject::radarProject(QString projectName,octavews* ws,bool newProject) : projectItem(nullptr)
{
    set_workspace(ws);
    _b_loading = true;
    load_project_file(projectName,newProject);
    _b_loading = false;

}
//---------------------------------------------
radarProject::~radarProject()
{
    QVector <opScheduler*> schedulers = get_available_scheduler();
    for (auto& scheduler : schedulers)
        if (scheduler != nullptr)
            delete scheduler;

    // Remove all radar instances
    QVector<radarInstance*> devices = get_available_radars();
    for (auto & device: devices)
        if (device != nullptr)
            delete device;

    QVector<radarModule*> modules = get_available_modules();
    for (auto & module : modules)
        if (module != nullptr)
            delete module;

    QVector<octaveScript*> scripts = get_available_scripts();
    for (auto & script : scripts)
        if (script != nullptr)
            delete script;

    QVector<antenna*>     antennas= get_available_antennas();
    for (auto & antenna: antennas)
        if (antenna!=nullptr)
            delete antenna;

    while (!qlChild.isEmpty())
    {
        auto & child = qlChild.last();
        if (child != nullptr)
            delete child;
    }

    qlChild.clear();

}
//---------------------------------------------


//---------------------------------------------
QString radarProject::get_last_error()
{
    return _last_error;
}
//----------------------------------------------
//----------------------------------------------
// When a radar module is updated via editor
//----------------------------------------------
//----------------------------------------------
void    radarProject::update_radar_module(radarModule* module, projectItem* pitem)
{
    QString dest_base_folder = get_full_path();
    folder* module_folder = (folder*)(get_folder(QString(cstr_radar_modules)));
    QString module_path = module_folder == nullptr  ? "" : module_folder->get_relative_path();
    folder *script_folder = (folder*)(get_folder(QString(cstr_scripts)));
    QString script_path = script_folder == nullptr ? "" :  script_folder->get_relative_path();
    folder *antenna_folder = (folder*)(get_folder(QString(cstr_antenna_models)));
    QString antenna_path =  antenna_folder == nullptr ? "" : antenna_folder->get_relative_path();

    // We need to check new scripts / antennas and update their paths according to the project
    module->copy_scripts_to_folder(dest_base_folder,script_path);
    module->copy_antenna_models_to_folder(dest_base_folder,antenna_path);
    // To be checked
    move_to_new_basedir(dest_base_folder,module_path,false,false);
    update_radar_module_devices(module);
    emit item_updated((projectItem*)module);
}
//----------------------------------------------
//----------------------------------------------
// Update radar module from a filename
//----------------------------------------------
//----------------------------------------------
void    radarProject::update_radar_module(QString &filename, projectItem* pitem)
{
    _last_error ="";
    QFileInfo fi(filename);

    if (!fi.exists()) return;

    if (pitem==nullptr)
        pitem = get_folder(QString(cstr_radar_modules));

    if (pitem==nullptr) return;

    // Identification is based on file basename
    QVector<radarModule*>   modules = get_available_modules();
    for (const auto& module : modules)
    {
        if (module==nullptr) continue;
        if (module->get_name() == fi.fileName())
        {
            if (!module->load_file(filename))
                _last_error = module->get_name() + ": error while re-loading radar module";

            update_radar_module(module, pitem);
            return;
        }
    }

    return;
}

//----------------------------------------------
//----------------------------------------------
// Add a radar module from a filename
//----------------------------------------------
//----------------------------------------------
void    radarProject::add_radar_module(QString &filename, projectItem* pitem)
{
    _last_error ="";

    QFileInfo fi(filename);

    if (!fi.exists()) return;

    if (pitem==nullptr)
        pitem = get_folder(QString(cstr_radar_modules));

    if (pitem==nullptr) return;

    // Identification is based on file basename
    QVector<radarModule*>   modules = get_available_modules();
    for (const auto& module : modules)
    {
        if (module==nullptr) continue;
        if (module->get_name() == fi.fileName())
        {
            _last_error = module->get_name() + " : Radar module already defined";
            return;
        }
    }

    //folder* module_folder = (folder*)(pitem);

    folder *script_folder = (folder*)get_folder(cstr_scripts);
    if (script_folder==nullptr) return;

    folder *antenna_folder = (folder*)get_folder(cstr_antenna_models);
    if (antenna_folder==nullptr) return;

    // Check if we have a valid radar module file

    radarModule* _radar_module = new radarModule(filename);
    folder * modules_home = (folder*)(get_folder(cstr_radar_modules));
    if (modules_home != nullptr)
        modules_home->add_child(_radar_module);

    if (_radar_module==nullptr) return;

    if (!_radar_module->load_file(filename))
    {
        if (modules_home!=nullptr)
        {
            modules_home->remove_child(_radar_module);

            if (_radar_module!=nullptr)
                delete _radar_module;
        }

        return;
    }
    // The requisite for importing a module into a project is that, anyway, underlying data
    // (scripts / antennas) are already added to the project.

/*
    // Copy the radar file structure into the project and update the internal data
    _radar_module->copy_to_new_folder(get_full_path(),
                                                                    module_folder->get_relative_path(),
                                                                    script_folder->get_relative_path(),
                                                                    antenna_folder->get_relative_path());

    pitem->add_child(_radar_module);

    add_module_scripts(_radar_module);
    add_module_antennas(_radar_module);
*/
    emit add_item(pitem, _radar_module);
}

//----------------------------------------------
void    radarProject::add_radar_module(radarModule* radar_module_ptr, projectItem* pitem)
{
    _last_error = "";

    if (radar_module_ptr == nullptr)
        return;

    if (pitem==nullptr)
        pitem = get_folder(QString(cstr_radar_modules));

    if (pitem==nullptr) return;

    // Identification is based on file basename
    QVector<radarModule*>   modules = get_available_modules();
    for (const auto& module : modules)
    {
        if (module==nullptr) continue;
        if (module->get_name() == radar_module_ptr->get_name())
        {
            _last_error = module->get_name() + " : Radar module already defined";
            return;
        }
    }
    radar_module_ptr->remove_temporary_project();
    pitem->add_child(radar_module_ptr);

    add_module_scripts(radar_module_ptr);
    add_module_antennas(radar_module_ptr);

    emit add_item(pitem, radar_module_ptr);
}
//----------------------------------------------
bool    radarProject::remove_radar_module(std::shared_ptr<radarModule> radar_module_to_remove, projectItem* pitem)
{
    return remove_radar_module(radar_module_to_remove.get(), pitem);
}
//----------------------------------------------
bool    radarProject::remove_radar_module(radarModule* radar_module_to_remove, projectItem* pitem)
{
    if (pitem==nullptr)
        pitem = this;

    _last_error = "";
    pitem = pitem->get_parent(radar_module_to_remove);

    if (pitem==nullptr) return false;

    QVector<radarInstance*> devices = get_available_radars();
    for (auto& device: devices)
    {
        if ((device!=nullptr)&&(device->get_module()==radar_module_to_remove))
        {
            _last_error = tr("Cannot delete module: device ")+device->get_device_name()+tr(" is defined");
            return false;
        }
    }
    if (!pitem->has_child(radar_module_to_remove)) return false;

    if (radar_module_to_remove!=nullptr)
    {
        pitem->remove_child(radar_module_to_remove);
    }

    emit remove_item(pitem, radar_module_to_remove);

    if (radar_module_to_remove!=nullptr)
        delete radar_module_to_remove;

    save_project_file();

    return true;
}
//----------------------------------------------
bool    radarProject::remove_radar_module(QString &radar_module_name, projectItem* pitem)
{
    if (pitem==nullptr)
        pitem = this;

    _last_error ="";

    projectItem *radar_to_del = pitem->get_child(radar_module_name, DT_RADARMODULE);

    if (radar_to_del==nullptr) return false;

    QVector<radarInstance*> devices = get_available_radars();
    for (auto& device: devices)
    {
        if ((device!=nullptr)&&(device->get_module()==radar_to_del))
        {
            _last_error = tr("Cannot delete module: device ")+device->get_device_name()+tr(" is defined");
            return false;
        }
    }

    // Check if we can remove: if we have devices then we don't remove

    pitem = pitem->get_parent(radar_to_del);
    if (pitem == nullptr) return false;

    if (!pitem->has_child(radar_to_del)) return false;

    if (radar_to_del==nullptr) return true;

    pitem->remove_child(radar_to_del);

    emit remove_item(pitem, radar_to_del);

    if (radar_to_del!=nullptr)
        delete radar_to_del;

    return true;
}
//--------------------------------------
void   radarProject::add_module_antennas(radarModule* module)
{
    if (module==nullptr) return;
    projectItem* root_antenna_models = get_folder(QString(cstr_antenna_models));
    if (root_antenna_models==nullptr) return;

    const QVector<antenna_model_pointer>& available_models = module->get_antenna_models();

     for (const auto& model:available_models)
        if (model!=nullptr)
        {
            // We may already have the model (maybe we uploaded different modules with same
            // antennas. So we skip this one.
            QString name = model->get_name();
          // Search if the current model is already present into the database
            if (root_antenna_models->get_child(name, DT_ANTENNA)!=nullptr)
                continue;

            // if not, add a new one
            root_antenna_models->add_child(model);
        }
}

//--------------------------------------
void   radarProject::add_module_scripts(radarModule* module)
{
    if (module==nullptr) return;

    projectItem* pitem =  get_folder(QString(cstr_scripts));
    if (pitem==nullptr) return;

    const QVector<octaveScript_ptr>& scripts  = module->get_init_scripts();

    for (const auto& script: scripts)
        add_script( (script), pitem );

    const QVector<octaveScript_ptr>& scripts_acq  = module->get_acquisition_scripts();
    for (const auto& script: scripts_acq)
        add_script( (script), pitem );

    const QVector<octaveScript_ptr>& scripts_post  = module->get_postacquisition_scripts();
    for (const auto& script: scripts_post)
        add_script( (script), pitem );
}
//----------------------------------------------
//----------------------------------------------
// Scripts
//----------------------------------------------
//----------------------------------------------
octaveScript*    radarProject::add_script(QString &filename, projectItem* pitem)
{
    _last_error = "";
    if (pitem==nullptr)
        pitem = get_folder(QString(cstr_scripts));

    QVector<octaveScript*> scripts = get_available_scripts();

    for (const auto& script : scripts)
    {
        if (script==nullptr) continue;
        if (script->get_name()==QFileInfo(filename).fileName())
        {
            _last_error = script->get_name() + " : Script already defined";
            return script;
        }
    }

    projectItem *script_folder =  get_folder(cstr_scripts);
    if (script_folder==nullptr) return nullptr;
    // Copy the file to the desination
    QFileInfo fdestination(QDir(script_folder->get_full_path()),QFileInfo(filename).fileName());
    QString new_file = fdestination.absoluteFilePath();

    if (filename != new_file)
        QFile(filename).copy(new_file);

    octaveScript* new_script = new octaveScript(new_file,script_folder);
    //Use only fileName since path is defined by the project tree
    new_script->set_filename(filename);

    emit add_item(script_folder,new_script);
    return new_script;
}

//-----------------------------------------------
void    radarProject::add_script(octaveScript* script_ptr, projectItem* pitem)
{
    _last_error ="";
    if (script_ptr==nullptr) return;

    if (pitem==nullptr)
        pitem = get_folder(QString(cstr_scripts));

    if (pitem==nullptr) return;

    QVector<octaveScript*> scripts = get_available_scripts();

    octaveScript temp(*script_ptr);

    QString base_dir = get_full_path();

    folder *script_folder = (folder*)get_folder(cstr_scripts);
    if (script_folder==nullptr) return;

    QString script_folder_dir = script_folder->get_relative_path();
    // Rebase the script
    temp.move_to_new_basedir(base_dir,script_folder_dir,false,true);

    for (const auto& script : scripts)
    {
        if (script==nullptr) continue;
        if (script->get_name()==script_ptr->get_name())
        {
            _last_error = script->get_name() + " : Script already defined";
            return;
        }
    }

    *script_ptr = temp;

    pitem->add_child(script_ptr);

    emit add_item(pitem, script_ptr);
}

//-----------------------------------------------
void    radarProject::remove_script(std::shared_ptr<octaveScript> script_to_remove, projectItem* pitem)
{
    if (script_to_remove==nullptr) return;

    if (pitem==nullptr)
        pitem = this;

    if (pitem==nullptr) return;

    projectItem *script_to_del = pitem->get_child(script_to_remove->get_name(), DT_SCRIPT);

    if (script_to_del==nullptr) return;

    // Check that script is not belonging

    pitem = pitem->get_parent(script_to_del);
    if (pitem == nullptr) return;

    if (!pitem->has_child(script_to_del)) return;

    if (script_to_del==nullptr) return;

    pitem->remove_child(script_to_del);

    emit remove_item(pitem, script_to_del);

    if (script_to_del!=nullptr)
        delete script_to_del;

}
//-----------------------------------------------
void    radarProject::remove_script(QString &radar_module_name, projectItem* pitem)
{
    if (pitem==nullptr)
        pitem = this;

    if (pitem==nullptr) return;

    projectItem *script_to_del = pitem->get_child(radar_module_name, DT_SCRIPT);

    if (script_to_del==nullptr) return;

    pitem = pitem->get_parent(script_to_del);
    if (pitem == nullptr) return;

    if (!pitem->has_child(script_to_del))  return;

    if (script_to_del==nullptr) return;

    pitem->remove_child(script_to_del);

    emit remove_item(pitem, script_to_del);

    if (script_to_del!=nullptr)
        delete script_to_del;


}

//----------------------------------------------
//----------------------------------------------
// Save
//----------------------------------------------
//----------------------------------------------

bool    radarProject::save_project_file()
{
    if (_b_loading) return true;

    if (_radar_project_file.isOpen())
        _radar_project_file.close();

    if (!_radar_project_file.open(QFile::WriteOnly | QFile::Text ))
        return false;

    QTextStream xmlContent(&_radar_project_file);

    QDomDocument document;

    xmlContent << document.toString();

    QDomElement root = document.createElement("ARIA_Radar_Project");

    QVersionNumber xml_ver({XML_ARIAPROJECT_MAJOR,XML_ARIAPROJECT_MINOR,XML_ARIAPROJECT_SUBVER});
    root.setAttribute("xml_dataformat", xml_ver.toString());

    save_xml(document, root);

    document.appendChild(root);

    xmlContent << document.toString();

    _radar_project_file.close();

    return true;
}

//-----------------------------------------------
bool    radarProject::load_project_file()
{
    QDomDocument document;

    if (!_radar_project_file.open(QIODevice::ReadOnly ))
    {
        return false;
    }
    document.setContent(&_radar_project_file);
    _radar_project_file.close();
    QDomElement root = document.firstChildElement("ARIA_Radar_Project");
    if (root.isNull()) return false;

    QString xml_ver_string = root.attribute("xml_dataformat");

    QVersionNumber xml_ver = QVersionNumber::fromString(xml_ver_string);

    QVersionNumber current_xml({XML_ARIAPROJECT_MAJOR,XML_ARIAPROJECT_MINOR, XML_ARIAPROJECT_SUBVER});

    if (xml_ver > current_xml)
        return false;
    QDomElement root_leaf = root.firstChildElement("leaf");
    if (root_leaf.isNull())
        return false;
    //
    QDomElement root_leaf_backup = root_leaf;
    // 1. Create root
    load_item(root_leaf, nullptr, nullptr, DT_ROOT);

    // 2. Create folders
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this,nullptr,DT_FOLDER);
    // 3. Load antenna models
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this, nullptr,DT_ANTENNA);
    // 3. Load scripts models
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this, nullptr,DT_SCRIPT);
    // 4. Load radar module
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this, nullptr,DT_RADARMODULE);
    // 5. Load radar devices
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this, nullptr, DT_RADARDEVICE);
    // 5. Load radar devices
    root_leaf = root_leaf_backup;
    load_item(root_leaf, this, nullptr, DT_SCHEDULER);

    return true;
}

//----------------------------------------------
//----------------------------------------------
// Load
//----------------------------------------------
//----------------------------------------------

bool    radarProject::load_project_file(QString filename, bool newProject)
{
    if (newProject)
    {
        // E.g. filename = /home/test/project/radarprj
        QFileInfo finfo(filename);
        QDir dir(finfo.absolutePath());
        // dir is /home/test/project/
        dir.setPath(dir.cleanPath(dir.absoluteFilePath(finfo.baseName())));
        // dir is /home/test/project/radarprj
        if (!dir.exists())
        {
            dir.setPath(finfo.absolutePath());
            dir.mkdir(finfo.baseName());
            dir.setPath(dir.cleanPath(dir.absoluteFilePath(finfo.baseName())));
        }

        QString base_dir = dir.absolutePath();
        // basedir is /home/test/project/radarprj

        move_to_new_basedir(base_dir,"",false,false);
        set_name(finfo.baseName());

        // filename must become /home/test/project/radarprj/radarprj.arp
        QFileInfo fi_out(base_dir, finfo.fileName());

        // We need to update the file name
        _filename = fi_out.absoluteFilePath();
        _radar_project_file.setFileName(_filename);

        // Create all children

        projectItem::create_new_item(this, new folder(cstr_radar_devices,base_dir));
        projectItem::create_new_item(this, new folder(cstr_scripts,base_dir));
        projectItem::create_new_item(this, new folder(cstr_handler,base_dir));
        projectItem::create_new_item(this, new folder(cstr_radar_modules,base_dir));
        projectItem::create_new_item(this, new folder(cstr_comm_protocol,base_dir));
        projectItem::create_new_item(this, new folder(cstr_antenna_models,base_dir));
        save_project_file();
    }
    else
    {
        clear();
        QFileInfo fi(filename);
        _filename = filename;
        set_name(fi.baseName());

        _radar_project_file.setFileName(_filename);

        // Create all children
        _b_loading = true;
        bool bRes = load_project_file();
        _b_loading = false;
        return bRes;
    }
    return true;
}

//-----------------------------------------------
void    radarProject::clear()
{
    this->remove_all_children();
}

//-----------------------------------------------
void    radarProject::add_radar_instance(const QByteArray& uid, radarModule* module)
{
    _last_error = "";
    radarInstance* radar = new radarInstance(module);
    if (radar == nullptr)
    {
        _last_error = "Error while creating new radar";
        return;
    }
    radar->attach_to_workspace(_workspace);
    radar->set_uid(uid);
    radar->set_fixed_id(true);
    QVector<radarInstance*> radars = get_available_radars();
    // Since we provide a UID, we assume the radar instance must be
   //  identified by the radar ID
    for (const auto& other: radars)
    {
        if (other==nullptr) continue;
        //
        if ((other->fixed_id())&&(other->get_uid()==uid))
        {
            _last_error = other->get_uid_string() + " : uid already defined";
            return;
        }
    }

    projectItem *pitem = get_folder(QString(cstr_radar_devices));

    if (pitem==nullptr) return;

    pitem->add_child(radar);

    emit add_item(pitem, radar);

    save_project_file();

}

void    radarProject::add_radar_instance(QString filename)
{
    radarInstance* device = new radarInstance();
    if (device==nullptr) return;
    device->attach_to_workspace(_workspace);
    device->set_filename(filename);
    device->set_temporary_project(this);

    if (!device->load_xml())
        delete device;
    else
    {
        device->remove_temporary_project();
        add_radar_instance(device);
    }

    save_project_file();
}
//-----------------------------------------------
void    radarProject::add_radar_instance(radarInstance* radar_ptr)
{
    _last_error = "";
    if (radar_ptr==nullptr) return;

    QVector<radarInstance*> radars = get_available_radars();
    for (const auto& other: radars)
    {
        if (other==nullptr) continue;

        if ((!(other->can_differentiate()))&&(!(radar_ptr->can_differentiate())))
            if ((radar_ptr->get_module()!=nullptr)&&(other->get_module()!=nullptr)&&
                (radar_ptr->get_module()->get_name()==other->get_module()->get_name()))
                {
                    _last_error = "Cannot have two different generic devices of the same model";
                    return;
                }

        if ((other->fixed_id())&&(radar_ptr->fixed_id())&&(other->get_uid()==other->get_uid()))
            if ((radar_ptr->get_module()!=nullptr)&&(other->get_module()!=nullptr)&&
                (radar_ptr->get_module()->get_name()==other->get_module()->get_name()))
                {
                    _last_error = other->get_uid_string() + " ID already defined for module " + other->get_module()->get_name();
                    return;
                }

        if ((other->fixed_port())&&(radar_ptr->fixed_port())&&
            (other->get_expected_portname()==radar_ptr->get_expected_portname()))
            if ((radar_ptr->get_module()!=nullptr)&&(other->get_module()!=nullptr)&&
                (radar_ptr->get_module()->get_name()==other->get_module()->get_name()))
                {
                    _last_error = other->get_expected_portname() + " port already allocated for module " + other->get_module()->get_name();
                    return;
                }
    }

//    radarInstance* radar = new radarInstance(*radar_ptr);

//    if (radar==nullptr) return;
    radar_ptr->attach_to_workspace(_workspace);

    projectItem *pitem = get_folder(QString(cstr_radar_devices));

    if (pitem==nullptr) return;

    pitem->add_child(radar_ptr);

    emit add_item(pitem, radar_ptr);

    save_project_file();
}

//-----------------------------------------------
void    radarProject::remove_radar_instance(radarInstance* radar_ptr, projectItem* pitem)
{
    if (pitem==nullptr)
        pitem = this;

    pitem = pitem->get_parent(radar_ptr);

    if (pitem==nullptr) return;

    if (!pitem->has_child(radar_ptr))  return;

    QVector<opScheduler*> scheds = get_available_scheduler();
    for (auto& sched: scheds)
    {
        if (sched==nullptr) continue;
        if (sched->has_device(radar_ptr))
            sched->delete_radar(radar_ptr);
    }

    if (radar_ptr==nullptr) return;

    pitem->remove_child(radar_ptr);

    emit remove_item(pitem, radar_ptr);

    if (radar_ptr!=nullptr)
        delete radar_ptr;

    save_project_file();
}
//-----------------------------------------------
void    radarProject::remove_radar_instance(QString radarName, projectItem* pitem)
{
    if (pitem==nullptr)
        pitem = this;

    QVector<radarInstance*> radars = get_available_radars();


    for (const auto& radar_ptr: radars)
        if (radar_ptr != nullptr)
        {
            if (radar_ptr->get_device_name() != radarName) continue;

            pitem = pitem->get_parent(radar_ptr);

            if (pitem==nullptr) return;

            QVector<opScheduler*> scheds = get_available_scheduler();

            for (auto& sched: scheds)
            {
                if (sched==nullptr) continue;
                if (sched->has_device(radar_ptr))
                    sched->delete_radar(radar_ptr);

            }

            pitem->remove_child(radar_ptr);

            emit remove_item(pitem, radar_ptr);

            if (radar_ptr !=nullptr)
                delete radar_ptr;

            break;
        }
    save_project_file();
}
//-----------------------------------------------
antenna* radarProject::add_antenna(QString filename)
{
    QVector <antenna*> ants = get_available_antennas();
    // If already existing, return the pointer of previous antenna model
    for (const auto& ant : ants)
        if (ant->get_full_filepath()==filename)
            return (antenna*)get_child(QFileInfo(filename).fileName(),DT_ANTENNA);

    projectItem *antenna_folder =  get_folder(cstr_antenna_models);
    if (antenna_folder==nullptr) return nullptr;
    // Copy the file to the desination
    QFileInfo fdestination(QDir(antenna_folder->get_full_path()),QFileInfo(filename).fileName());
    QString new_file = fdestination.absoluteFilePath();

    if (filename != new_file)
        QFile(filename).copy(new_file);


    antenna* new_antenna = new antenna();
    if (new_antenna==nullptr) return nullptr;
    if (!(new_antenna->load(new_file)))
    {
        delete new_antenna;
        return nullptr;
    }
    //Use only fileName since path is defined by the project tree
    new_antenna->set_filename(QFileInfo(filename).fileName());
    antenna_folder ->add_child(new_antenna);
    emit add_item(antenna_folder,new_antenna);

    save_project_file();
    return new_antenna;
}
//-----------------------------------------------
antenna* radarProject::add_antenna(antenna* instance)
{
    QVector <antenna*> ants = get_available_antennas();
    // If already existing, return the pointer of previous antenna model
    for (const auto& ant : ants)
        if (ant->get_full_filepath()==instance->get_name())
            return (antenna*)get_child(instance->get_name(),DT_ANTENNA);

    projectItem *antenna_folder =  get_folder(cstr_antenna_models);
    if (antenna_folder==nullptr) return nullptr;
    // Copy the file to the desination
    QString originalFileName = instance->get_full_filepath();
    QFileInfo fdestination(QDir(antenna_folder->get_full_path()),QFileInfo(originalFileName).fileName());
    QString new_file = fdestination.absoluteFilePath();
    if (originalFileName != new_file)
        QFile(originalFileName).copy(new_file);


    antenna* new_antenna = new antenna();
    if (new_antenna==nullptr) return nullptr;
    if (!(new_antenna->load(new_file)))
    {
        delete new_antenna;
        return nullptr;
    }
    //Use only fileName since path is defined by the project tree
    new_antenna->set_filename(QFileInfo(originalFileName).fileName());
    antenna_folder ->add_child(new_antenna);
    emit add_item(antenna_folder,new_antenna);

    save_project_file();
    return new_antenna;
}
//-----------------------------------------------
void         radarProject::remove_antenna(QString filename)
{
    _last_error = "";
    // Check if any module has this antenna. If so, block deleting
    QVector<radarModule*> modules = get_available_modules();

    for (auto& module: modules)
        if(module!=nullptr)
        {
            antenna_array array = module->get_antenna_array();
            for (auto& ant: array)
                if (ant!=nullptr)
                {
                    if (ant->get_antenna_model()->get_filename() == filename)
                    {
                        _last_error = tr("Cannot delete antenna since it belongs to module")+module->get_name();
                        return;
                    }
                }
        }


    QVector<antenna*> ants = get_available_antennas();
    projectItem* ant_folder = get_folder(cstr_antenna_models);
    for (const auto& ant : ants)
    {
        if (QFileInfo(ant->get_full_filepath()).fileName()==filename)
        {
            if (!ant_folder->has_child(ant)) return;
            ant_folder->remove_child(ant);
            emit remove_item(ant_folder, ant);
            delete ant;
            return;
        }
    }
    save_project_file();
}
//-----------------------------------------------
void    radarProject::update_radar_module_devices(projectItem* moduleitem)
{
    if (moduleitem==nullptr) return;
    if (moduleitem->get_type()!=DT_RADARMODULE) return;

    radarModule* module = (radarModule*)moduleitem;
    QVector<radarInstance*> devices = get_available_radars();

    for (auto& device : devices)
    {
        if (device == nullptr) continue;
        if (device->get_module() != module) continue;
        device->update_module();
    }
}

//-----------------------------------------------
/*
void    radarProject::variable_updated(const std::string& varname)
{
    QVector<radarInstance*> devices = get_available_radars();
    for (auto& device:devices)
        if (device!=nullptr)
            device->update_variable(varname);
}
*/
//-----------------------------------------------
void    radarProject::variables_updated(const std::set<std::string>& varlist)
{
    QVector<radarInstance*> devices = get_available_radars();
    for (auto& device:devices)
        if (device!=nullptr)
            device->update_variables(varlist);
}


//-----------------------------------------------
void     radarProject::add_scheduler(opScheduler* scheduler)
{
    _last_error = "";

    projectItem*  pitem = get_folder(QString(cstr_handler));

    if (pitem == nullptr) return;

    QVector<opScheduler*> schedulers = get_available_scheduler();

    for (const auto& sched : schedulers)
    {
        if (sched==nullptr) continue;
        if (sched==scheduler)
        {
            _last_error = sched->get_name() + " : Script already defined";
            return;
        }
    }

    pitem->add_child(scheduler);
    emit add_item(pitem,scheduler);

    save_project_file();
    return;
}
//-----------------------------------------------
void     radarProject::remove_scheduler(opScheduler* scheduler, projectItem* pitem)
{
    if (pitem == nullptr)
        pitem = get_folder(cstr_handler);

    if (pitem==nullptr)
        return;

    if (!pitem->has_child(scheduler)) return;

    pitem->remove_child(scheduler);

    emit remove_item(pitem, scheduler);

    if (scheduler!=nullptr)
        delete scheduler;

    save_project_file();
}
//-----------------------------------------------
opScheduler*    radarProject::add_scheduler(QString filename)
{
    _last_error = "";

    projectItem* pitem = get_folder(QString(cstr_handler));

    if (pitem==nullptr)
    {
        _last_error = "Missing scheduler folder";
        return nullptr;
    }
    QVector<opScheduler*> schedulers = get_available_scheduler();

    for (const auto& scheduler : schedulers)
    {
        if (scheduler==nullptr) continue;
        if (scheduler->get_name()==QFileInfo(filename).fileName())
        {
            _last_error = scheduler->get_name() + " : Script already defined";
            return scheduler;
        }
    }
    // Copy the file to the desination
    QFileInfo fdestination(QDir(pitem->get_full_path()),QFileInfo(filename).fileName());
    QString new_file = fdestination.absoluteFilePath();

    if (filename != new_file)
        QFile(filename).copy(new_file);
    opScheduler* new_scheduler = new opScheduler(_workspace->data_interface());

    pitem->add_child(new_scheduler);
        //Use only fileName since path is defined by the project tree
    new_scheduler->set_filename(filename);

    emit add_item(pitem,new_scheduler);

    save_project_file();

    return new_scheduler;
}



