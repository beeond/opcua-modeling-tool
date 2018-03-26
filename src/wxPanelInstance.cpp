#include <wxPanelInstance.h>

#include <opcutils.h>
#include <synthesis/opc.hxx>
#include <wxPanelNode.h>
#include <wxFrameMain.h>
#include <wxTreeDialog.h>

#include <vector>
#include <wx/msgdlg.h>
#include <wx/log.h>

//(*InternalHeaders(wxPanelInstance)




//(*IdInit(wxPanelInstance)








BEGIN_EVENT_TABLE(wxPanelInstance,wxPanel)
	//(*EventTable(wxPanelInstance)
	//*)
END_EVENT_TABLE()

using namespace std;

wxPanelInstance::wxPanelInstance(wxWindow* parent,wxWindowID id,const wxPoint& pos,const wxSize& size)
{
	//(*Initialize(wxPanelInstance)
















































    this->m_mainSizer               = panelMainSizer;
    this->m_nodeSizer               = pnlNodeSizer;
    this->m_instanceSizer           = instanceSizer;
    this->m_instanceInternalSizer   = instanceInternalSizer;
    this->m_boxSizerTypeDefinition  = boxSizerTypeDefinition;

    vector<wxString> cboList;
    cboList.push_back("None");
    cboList.push_back("Mandatory");
    cboList.push_back("Optional");
    cboList.push_back("ExposesItsArray");
    cboList.push_back("CardinalityRestriction");
    cboList.push_back("MandatoryShared");
    cboList.push_back("OptionalPlaceholder");
    cboList.push_back("MandatoryPlaceholder");
	this->cboModeling->Set(cboList);
    m_typeDefNodeSelected = 0;
}

wxPanelInstance::~wxPanelInstance()
{
	//(*Destroy(wxPanelInstance)
	//*)
}

void wxPanelInstance::Init(InstanceDesign *instance, ModelDesign *model, wxTreeItemId treeItemId, wxFrameMain *mainFrame,
                           bool userOwner, enum NodeType m_nodeType,
                           bool hideEnum, bool hideChildren, bool hideRefereces, bool hideInstance)
{
    this->m_instance    = instance;
    this->m_model       = model;
    this->m_treeItemId  = treeItemId;
    this->m_mainFrame   = mainFrame;
    this->m_nodeType    = m_nodeType;

    this->panelNode->Init(instance, model, treeItemId, mainFrame, userOwner, hideEnum, hideChildren, hideRefereces);

    this->ShowTypeDefinition(true);

    //TODO: What panel is hiding the instance detail?
    if (hideInstance)
        this->m_instanceSizer->Hide(this->m_instanceInternalSizer, true);
    else
        this->m_instanceSizer->Show(this->m_instanceInternalSizer, true);

    this->m_instanceInternalSizer->Layout();
    this->m_instanceSizer->Layout();
    this->m_nodeSizer->Layout();
    this->m_mainSizer->Layout();

    //Disable interactive controls for built-in.
    this->cboModeling->Enable(userOwner);
    this->btnLookup->Enable(userOwner);
    this->txtTypeDefinition->Enable(userOwner);
    //this->txtDeclaration->Enable(userOwner);

    this->Clear();
}

void wxPanelInstance::Clear()
{
    //TODO: Populate TypeDefinition values.
    //      When a new type is added, how do we update the selection items? When dropdown clicked?
    //      Answer - Right click Refresh should be added to any node or the virtual root at least.
    this->cboModeling->SetValue("");
    this->txtTypeDefinition->SetValue("");
    //this->txtDeclaration->SetValue("");

    this->panelNode->Clear();
}

//TODO:
void wxPanelInstance::PopulateData(const enum NodeType nodeType)
{
    ListOfChildren childrenList;
    ListOfReferences referencesList;
    NodeDesign::Children_optional children(childrenList);
    NodeDesign::References_optional references(referencesList);

    int indexFoundForTypeDef = -1;
    int objectIndex          = 0;
    wxString typeDef         = OPCUtils::GetName<InstanceDesign::TypeDefinition_optional>(this->m_instance->TypeDefinition(), "");
    this->txtTypeDefinition->SetValue(typeDef);

    switch (nodeType)
    {
        case NodeTypeMethod:
        case NodeTypeObject:
        case NodeTypeChildObject:
            {
               ITERATE_MODELLIST(ObjectType, i, m_model)
                {
                    wxString symName = OPCUtils::GetName<NodeDesign::SymbolicName_optional>(i->SymbolicName());

                    //Find the ObjectType that the Object has typedef'd from.
                    if (    -1 == indexFoundForTypeDef
                         &&  0 != this->m_instance->TypeDefinition()
                         &&  0 == symName.compare(typeDef) )
                    {
                        indexFoundForTypeDef = objectIndex;
                        this->CopyParent(&(*i), &children, &references);
                    }
                    ++objectIndex;
                }
            }

        case NodeTypeChildVariable:
            {
                ITERATE_MODELLIST(VariableType, i, m_model)
                {
                    wxString symName = OPCUtils::GetName<NodeDesign::SymbolicName_optional>(i->SymbolicName());

                    //Find the VariableType that the Variable has typedef'd from.
                    if (    -1 == indexFoundForTypeDef
                         &&  0 != this->m_instance->TypeDefinition()
                         &&  0 == symName.compare(typeDef) )
                    {
                        indexFoundForTypeDef = objectIndex;
                        this->CopyParent(&(*i), &children, &references);
                    }
                    ++objectIndex;
                }
            }
        case NodeTypeChildProperty:
            {
                //N.B:  Property does not use a TypeDef, only DataType.
                //      I don't think we need to populate chidlren and references.
            }
        default:
            cerr << "Error: wxPanelInstance::PopulateData() case:" << nodeType << " not handled.\n";
    }
    //TODO: Other model element type

    wxString modelingRule  = OPCUtils::GetString(this->m_instance->ModellingRule());
    if (modelingRule.compare(NO_VALUE) !=0)
        this->cboModeling->SetValue(modelingRule);
    else
        this->cboModeling->SetValue("");

    this->panelNode->PopulateData();
	this->panelNode->PopulateChildren(&children);
	this->panelNode->PopulateReferences(&references);
}


void wxPanelInstance::CopyParent(TypeDesign *type, NodeDesign::Children_optional *inheritedChildren, NodeDesign::References_optional *inheritedReferences)
{
    //TODO: Add a logic to check for cyclic BaseType where a parent inherits a Child.
    //      Store baseType in a map, if key is there already then we terminate recursion and show message

    this->panelNode->CopyParent(type, inheritedChildren, inheritedReferences);

    //Get the BaseType and search the actual object
    wxString baseType = OPCUtils::GetName<TypeDesign::BaseType_optional>(type->BaseType(), "BaseObjectType");

    //Recursion will stop at the BaseObjectType
    if (baseType.Cmp("BaseObjectType") == 0)
        return;

    TypeDesign *parentType = 0;

    //Search from ObjectType
    for (ModelDesign::ObjectType_iterator i (this->m_model->ObjectType().begin());
         i != this->m_model->ObjectType().end ();
         ++i)
    {
        wxString symName = OPCUtils::GetName<NodeDesign::SymbolicName_optional>(i->SymbolicName());

        //Find the ObjectType that the Object has typedef'd from.
        if (0 == symName.compare(baseType))
        {
            parentType = &(*i);
            break;
        }
    }

    //TODO: Continue searching with the rest of the model element type if parentType is not null.

    //Search from VariableType

    //Search from PropertyType

    //Search from DataType

    //Search from ReferenceType?


    if (parentType != 0)
        this->CopyParent(parentType, inheritedChildren, inheritedReferences);
}

bool wxPanelInstance::UpdateData(const char *nodePrefix)
{
    bool retVal = false;

    //TODO: Update own fields

    string modelingRule = this->cboModeling->GetValue().ToStdString();
    if (modelingRule.compare("") != 0)
        this->m_instance->ModellingRule(modelingRule);

    wxString currentTypeDef = OPCUtils::GetName<InstanceDesign::TypeDefinition_optional>(this->m_instance->TypeDefinition(), "");
    if (currentTypeDef != this->txtTypeDefinition->GetValue().ToStdString())
        OPCUtils::SetTypeDef(*this->m_instance, this->txtTypeDefinition->GetValue().ToStdString(),
                             OPCUtils::IsUserNodeOwner(m_typeDefNodeSelected));


    retVal = this->panelNode->UpdateData(nodePrefix);
    //if (retVal)
        //TODO: Update fields here

    return retVal;
}


void wxPanelInstance::OnbtnLookupClick(wxCommandEvent& event)
{
    short int rootMask = 0;
    switch (this->m_nodeType)
    {
        case NodeTypeObject:
        case NodeTypeChildObject:
            {
                rootMask = RootMaskShowObjectType;
                break;
            }
        case NodeTypeChildVariable:
            {
                rootMask = RootMaskShowVariableType;
                break;
            }
        case NodeTypeChildProperty:
            {
                rootMask = RootMaskShowDataType; //Property uses the DataType for lookup of its Type Definition.
                break;
            }
        case NodeTypeMethod:
        case NodeTypeChildMethod:
            {
                rootMask = RootMaskShowMethod;
                break;
            }
        default:
            cerr << "switch default - This is a bug! case: " << this->m_nodeType << " not handled.\n";
    }
    cerr << "this->m_nodeType:%d" <<  this->m_nodeType << "\n";

    m_typeDefNodeSelected = 0;
    wxTreeDialog treeDlg(this, m_model, false, false, rootMask);
    treeDlg.ShowModal();

    if (treeDlg.GetReturnCode() == 0)
    {
        this->txtTypeDefinition->SetValue(treeDlg.SelectedSymbolicName);

        this->panelNode->DeleteChildren();
        this->panelNode->DeleteReferences();

        ListOfChildren childrenList;
        ListOfReferences referencesList;
        NodeDesign::Children_optional children(childrenList);
        NodeDesign::References_optional references(referencesList);
        m_typeDefNodeSelected = treeDlg.GetSelectedNode();

        TypeDesign* type = dynamic_cast<TypeDesign*>(treeDlg.GetSelectedNode());
        if (type != 0)
            {
            this->CopyParent(type, &children, &references);
            //Re-create the node because the type has changed, so as its inherited children and references
            //make up a sub-tree.
            this->m_mainFrame->ReCreateNodeTree(this->m_treeItemId, type, true); //@param userOwner is always true because button has been enabled.

            this->panelNode->PopulateChildren(&children);
            this->panelNode->PopulateReferences(&references);
            }
    }
}

void wxPanelInstance::ShowTypeDefinition(bool show)
{
    if (show)
    {
        this->m_boxSizerTypeDefinition->Show(this->lblTypeDefinition, true);
        this->m_boxSizerTypeDefinition->Show(this->txtTypeDefinition, true);
        this->m_boxSizerTypeDefinition->Show(this->btnLookup, true);
    }
    else
    {
        this->m_boxSizerTypeDefinition->Hide(this->lblTypeDefinition, true);
        this->m_boxSizerTypeDefinition->Hide(this->txtTypeDefinition, true);
        this->m_boxSizerTypeDefinition->Hide(this->btnLookup, true);
    }
}