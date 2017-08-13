#pragma once

#include "RZBConfiguration.h"

namespace TestUI {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Security;
	using namespace System::Security::AccessControl;
	using namespace Microsoft::Win32;

	/// <summary>
	/// Summary for Settings
	/// </summary>
	public ref class Settings : public System::Windows::Forms::Form
	{
	public:
		Settings(void)
		{
			//
			//TODO: Add the constructor code here
			//

			InitializeComponent();

			//Connect to Registry
			//HKCU/Software/Razorback/API
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Settings()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		RegistryKey ^HKLM_Software;
		RegistryKey ^rzbkey;
		RegistryKey ^defaultkey;
		array <RZBConfiguration ^> ^configurations;
		RZBConfiguration ^lastSelectedConf;
		array <String^>^ configurationNames;
		array <System::Windows::Forms::TreeNode^> ^rootNodes;

	private: RegistryKey^ rzbapikey;
	private: System::Windows::Forms::Panel^  panel1;
	protected: 
	private: System::Windows::Forms::TreeView^  treeView1;
	private: System::Windows::Forms::GroupBox^ currentVisible;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::Button^  button3;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Settings::typeid));
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->treeView1 = (gcnew System::Windows::Forms::TreeView());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->panel1->SuspendLayout();
			this->SuspendLayout();
			// 
			// panel1
			// 
			this->panel1->BackColor = System::Drawing::SystemColors::Window;
			this->panel1->Controls->Add(this->treeView1);
			this->panel1->Location = System::Drawing::Point(12, 12);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(221, 551);
			this->panel1->TabIndex = 0;
			// 
			// treeView1
			// 
			this->treeView1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->treeView1->HotTracking = true;
			this->treeView1->Location = System::Drawing::Point(0, 0);
			this->treeView1->Name = L"treeView1";
			this->treeView1->Size = System::Drawing::Size(221, 551);
			this->treeView1->TabIndex = 0;
			this->treeView1->AfterSelect += gcnew System::Windows::Forms::TreeViewEventHandler(this, &Settings::treeView1_AfterSelect);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(273, 524);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(122, 38);
			this->button1->TabIndex = 20;
			this->button1->Text = L"Restore Defaults";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Settings::button1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(585, 524);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(122, 38);
			this->button2->TabIndex = 21;
			this->button2->Text = L"Save Changes";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &Settings::button2_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(713, 524);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(122, 38);
			this->button3->TabIndex = 22;
			this->button3->Text = L"Revert";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &Settings::button3_Click);
			// 
			// Settings
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(896, 574);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->panel1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->Name = L"Settings";
			this->Text = L"Settings";
			this->Load += gcnew System::EventHandler(this, &Settings::Settings_Load);
			this->panel1->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion

	private: System::Void treeView1_AfterSelect(System::Object^  sender, System::Windows::Forms::TreeViewEventArgs^  e) {
				 RZBConfiguration ^conf;
				 
				 if (e->Node->Parent != nullptr) {

					conf = static_cast<RZBConfiguration ^>(e->Node->Parent);

					switch (e->Node->Index) {
						case 0:
							if (this->currentVisible != nullptr)
								this->currentVisible->Visible = false;	
			                this->currentVisible = conf->GlobalGroupBox;
							conf->GlobalGroupBox->Visible = true;
							break;
						
						case 1: 
							if (this->currentVisible != nullptr)
								this->currentVisible->Visible = false;
							this->currentVisible = conf->CacheGroupBox;
							conf->CacheGroupBox->Visible = true;
							break;

						case 2:
							if (this->currentVisible != nullptr)
								this->currentVisible->Visible = false;
							this->currentVisible = conf->LocalityGroupBox;
							conf->LocalityGroupBox->Visible = true;
							break;

						case 3:
							if (this->currentVisible != nullptr)
								this->currentVisible->Visible = false;
							this->currentVisible = conf->MessageQueueGroupBox;
							conf->MessageQueueGroupBox->Visible = true;
							break;

						case 4:
							if (this->currentVisible != nullptr)
								this->currentVisible->Visible = false;
							this->currentVisible = conf->LoggingGroupBox;
							conf->LoggingGroupBox->Visible = true;
							break;
					}
				}
				else {
					conf = static_cast<RZBConfiguration ^>(e->Node);
				}

				lastSelectedConf = conf;
			}

private: System::Void Settings_Load(System::Object^  sender, System::EventArgs^  e) {
		
			try {
				HKLM_Software = Registry::LocalMachine->OpenSubKey("SOFTWARE", true);
				if (!HKLM_Software)
					throw;
			}
			catch (Exception ^e) {
			    MessageBox::Show("Could not access registry key HKLM\\SOFTWARE");
				Application::Exit();
			}

			try {
				rzbkey = HKLM_Software->OpenSubKey("Razorback", true);
				if (!rzbkey)
					throw;
			} 
		    catch (Exception ^e) {
				rzbkey = HKLM_Software->CreateSubKey("Razorback");
				if (!rzbkey) {
					MessageBox::Show("Key Razorback not found. Could not create it.");
					Application::Exit();
				}
			}

			try {
				configurationNames = rzbkey->GetSubKeyNames();
			}
			catch (Exception ^e) {
				MessageBox::Show("Failed to get configurations");
				Application::Exit();
			}

			configurations = gcnew array <RZBConfiguration ^>(configurationNames->Length);		

			for (int i = 0; i < configurationNames->Length; i++) {
				//Open each configuration and populate it
				if (String::Compare(configurationNames[i], L"default") == 0) {
					try {
						defaultkey = rzbkey->OpenSubKey("default", true);
						if (!defaultkey)
							throw;
						configurations[i] = gcnew RZBConfiguration("api.conf", defaultkey);
						configurationNames[i] = "api.conf";
					}
					catch (Exception ^e) {
						MessageBox::Show("Could not get default configuration.");
					}
				}
				else {
					configurations[i] = gcnew RZBConfiguration(configurationNames[i], rzbkey);
				}

				this->treeView1->Nodes->Add(configurations[i]);
				this->Controls->Add(configurations[i]->GlobalGroupBox);
				this->Controls->Add(configurations[i]->CacheGroupBox);
				this->Controls->Add(configurations[i]->LocalityGroupBox);
				this->Controls->Add(configurations[i]->MessageQueueGroupBox);
				this->Controls->Add(configurations[i]->LoggingGroupBox);
			}

			this->currentVisible = nullptr;
			this->lastSelectedConf = nullptr;
		 }
private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			  RZBConfiguration ^conf = lastSelectedConf;

			  if (conf != nullptr) {
				  conf->populateDefaults(); 
				  conf->populateGlobalGroup();
				  conf->populateCacheGroup();
				  conf->populateLocalGroup();
				  conf->populateMQGroup();
				  conf->populateLogGroup();
			  }

		 }

private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
			  
			 if ((MessageBox::Show("Save changes for this configuration?", "Just checking.", MessageBoxButtons::YesNo,
				  MessageBoxIcon::Question)) == System::Windows::Forms::DialogResult::Yes)
			  {
				  lastSelectedConf->storeAll();
			  }
		 }

private: System::Void button3_Click(System::Object^  sender, System::EventArgs^  e) {
			 //Revert
			  RZBConfiguration ^conf = lastSelectedConf;

			  if (conf != nullptr) {
				  conf->populateData();
				  conf->populateGlobalGroup();
				  conf->populateCacheGroup();
				  conf->populateLocalGroup();
				  conf->populateMQGroup();
				  conf->populateLogGroup();
			  }

		 }
};
}
