#include "StdAfx.h"
#include "RZBConfiguration.h"

namespace TestUI {

	using namespace Microsoft::Win32;
	using namespace System::Windows::Forms;
	using namespace System;

RZBConfiguration::RZBConfiguration(String ^name, RegistryKey ^key)
{
	
	keyname = name;
	basekey = key;
	
	populateData();
	
	
	GlobalNode = gcnew System::Windows::Forms::TreeNode();
	GlobalNode->Name = L"GlobalNode";
	GlobalNode->Text = L"Global";
	CacheNode = gcnew System::Windows::Forms::TreeNode();
	CacheNode->Name = L"CacheNode";
	CacheNode->Text = L"Cache";
	LocalityNode = gcnew System::Windows::Forms::TreeNode();
	LocalityNode->Name = L"LocalityNode";
	LocalityNode->Text = L"Locality";
	MQNode = gcnew System::Windows::Forms::TreeNode();
	MQNode->Name = L"MQNode";
	MQNode->Text = L"Message Queue";
	LoggingNode = gcnew System::Windows::Forms::TreeNode();
	LoggingNode->Name = L"LoggingNode";
	LoggingNode->Text = L"Logging";
	Name = keyname;
	Text = keyname;

	Nodes->AddRange(gcnew cli::array <System::Windows::Forms::TreeNode^>(5) {GlobalNode, CacheNode, LocalityNode, MQNode, LoggingNode} );
	
	GlobalGroupBox = (gcnew System::Windows::Forms::GroupBox());
	CacheGroupBox = (gcnew System::Windows::Forms::GroupBox());
	LoggingGroupBox = (gcnew System::Windows::Forms::GroupBox());
	MessageQueueGroupBox = (gcnew System::Windows::Forms::GroupBox());
	LocalityGroupBox = (gcnew System::Windows::Forms::GroupBox());	

	MaxGoodLabel = (gcnew System::Windows::Forms::Label());
	MaxBadLabel = (gcnew System::Windows::Forms::Label());
	MaxGoodTextBox = (gcnew System::Windows::Forms::TextBox());
	MaxBadTextBox = (gcnew System::Windows::Forms::TextBox());
	
	SSLPanel = (gcnew System::Windows::Forms::Panel());
	SSLDisabledRadio = (gcnew System::Windows::Forms::RadioButton());
	SSLEnabledRadio = (gcnew System::Windows::Forms::RadioButton());
	SSLLabel = (gcnew System::Windows::Forms::Label());
	UsernameTextBox = (gcnew System::Windows::Forms::TextBox());
	PasswordTextBox = (gcnew System::Windows::Forms::TextBox());
	PortTextBox = (gcnew System::Windows::Forms::TextBox());
	HostTextBox = (gcnew System::Windows::Forms::TextBox());
	PasswordLabel = (gcnew System::Windows::Forms::Label());
	UsernameLabel = (gcnew System::Windows::Forms::Label());
	PortLabel = (gcnew System::Windows::Forms::Label());
	HostLabel = (gcnew System::Windows::Forms::Label());
	
	HashTypePanel = (gcnew System::Windows::Forms::Panel());
	SHA512Radio = (gcnew System::Windows::Forms::RadioButton());
	SHA256Radio = (gcnew System::Windows::Forms::RadioButton());
	SHA224Radio = (gcnew System::Windows::Forms::RadioButton());
	HashTypeLabel = (gcnew System::Windows::Forms::Label());
	SHA1Radio = (gcnew System::Windows::Forms::RadioButton());
	MD5Radio = (gcnew System::Windows::Forms::RadioButton());
	MsgFormatPanel = (gcnew System::Windows::Forms::Panel());
	MsgFormatLabel = (gcnew System::Windows::Forms::Label());
	JSONRadio = (gcnew System::Windows::Forms::RadioButton());
	BinaryRadio = (gcnew System::Windows::Forms::RadioButton());
	DeadLabel = (gcnew System::Windows::Forms::Label());
	HelloLabel = (gcnew System::Windows::Forms::Label());
	DeadTextBox = (gcnew System::Windows::Forms::TextBox());
	HelloTextBox = (gcnew System::Windows::Forms::TextBox());
	MaxBlockLabel = (gcnew System::Windows::Forms::Label());
	MaxBlockTextBox = (gcnew System::Windows::Forms::TextBox());
	MaxThreadsLabel = (gcnew System::Windows::Forms::Label());
	MaxThreadsTextBox = (gcnew System::Windows::Forms::TextBox());
	TransferPasswordLabel = (gcnew System::Windows::Forms::Label());
	TransferPasswordTextBox = (gcnew System::Windows::Forms::TextBox());
	
	BackupOrderLabel = (gcnew System::Windows::Forms::Label());
	StoragePathLabel = (gcnew System::Windows::Forms::Label());
	StoragePathTextBox = (gcnew System::Windows::Forms::TextBox());
	BackupOrderTextBox = (gcnew System::Windows::Forms::TextBox());
	LocalityIDLabel = (gcnew System::Windows::Forms::Label());
	LocalityIDTextBox = (gcnew System::Windows::Forms::TextBox());
	
	LogFilePathLabel = (gcnew System::Windows::Forms::Label());
	SyslogFacilityLabel = (gcnew System::Windows::Forms::Label());
	LogLevelLabel = (gcnew System::Windows::Forms::Label());
	DestinationLabel = (gcnew System::Windows::Forms::Label());
	LogFilePathTextBox = (gcnew System::Windows::Forms::TextBox());
	LogLevelComboBox = (gcnew System::Windows::Forms::ComboBox());
	SyslogFacilityComboBox = (gcnew System::Windows::Forms::ComboBox());
	DestinationComboBox = (gcnew System::Windows::Forms::ComboBox());
	
	CacheGroupBox->SuspendLayout();
	MessageQueueGroupBox->SuspendLayout();
	SSLPanel->SuspendLayout();
	GlobalGroupBox->SuspendLayout();
	HashTypePanel->SuspendLayout();
	MsgFormatPanel->SuspendLayout();
	LocalityGroupBox->SuspendLayout();
	LoggingGroupBox->SuspendLayout();
	
	initGlobalGroup();
	initCacheGroup();
	initLocalGroup();
	initMQGroup();
	initLogGroup();

	GlobalGroupBox->ResumeLayout();

}

void RZBConfiguration::populateDefaults() {

		MaxThreadsValue = "100";
		MaxBlockSizeValue = "100";
		TransferPasswordValue = "razorback";
		HashTypeValue = "SHA256";
		HelloTimeValue = "2";
		DeadTimeValue = "10";
		MessageFormatValue = "json";
		LocalityIDValue = "1";
		BlockStoreValue = "z:\\";
		BackupOrderValues = gcnew array <String ^> {"2", "3", "4"};
		BadLimitValue = "256";
		GoodLimitValue = "256";
		HostValue = "localhost";
		PortValue = "61612";
		UsernameValue = "User";
		PasswordValue = "Password";
		SSLValue = "False";
		DestinationValue = "stderr";
		LogLevelValue = "debug";
		SyslogFacilityValue = "daemon";
		FilePathValue = "";
}

void RZBConfiguration::populateData()
{
	try {
		rzbkey = basekey->OpenSubKey(keyname, true);
		if (!rzbkey)
			throw;
	} 
	catch (Exception ^e) {
		MessageBox::Show("Unable to open configuration %S.", keyname);
	}

	System::Object ^obj;

	try {
		if((obj = rzbkey->GetValue("Global.MaxThreads")) == nullptr)
			throw;
		MaxThreadsValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.MaxThreads", 100);
		MaxThreadsValue = "100";
	} 


	try {
		if ((obj = rzbkey->GetValue("Global.MaxBlockSize")) == nullptr)
			throw;
		MaxBlockSizeValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.MaxBlockSize", 100);
		MaxBlockSizeValue = "100";
	}



	try {
		if ((obj = rzbkey->GetValue("Global.HashType")) == nullptr)
			throw;
		HashTypeValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.HashType", "SHA256");
		HashTypeValue = "SHA256";
	}

	try {
		if ((obj = rzbkey->GetValue("Global.HelloTime")) == nullptr)
			throw;
		HelloTimeValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.HelloTime", 2);
		HelloTimeValue = "2";
	}



	try {
		if ((obj = rzbkey->GetValue("Global.DeadTime")) == nullptr)
			throw;
		DeadTimeValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.DeadTime", 10);
		DeadTimeValue = "10";
	}

	try {
		if ((obj = rzbkey->GetValue("Global.MessageFormat")) == nullptr)
			throw;
		MessageFormatValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.MessageFormat", "json");
		MessageFormatValue = "json";
	}



	try {
		if ((obj = rzbkey->GetValue("Global.TransferPassword")) == nullptr)
			throw;
		TransferPasswordValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Global.TransferPassword", "razorback");
		TransferPasswordValue = "razorback";
	}

	try {
		if ((obj = rzbkey->GetValue("Locality.Id")) == nullptr)
			throw;
		LocalityIDValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Locality.Id", 1);
		LocalityIDValue = "1";
	}

	try {
		if ((obj = rzbkey->GetValue("Locality.BlockStore")) == nullptr)
			throw;
		BlockStoreValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Locality.BlockStore", "z:\\");
		BlockStoreValue = "z:\\";
	}

	RegistryKey ^BOKey;
	
	try {
		BOKey = rzbkey->OpenSubKey("Locality.BackupOrder", true);
		if (!BOKey)
			throw;
	} 
	catch (Exception ^e) {
		MessageBox::Show("Unable to open configuration for Back-up Order.", keyname);
		//Restore defaults
	}

	subvalues = BOKey->GetValueNames();

	if (subvalues->Length > 0) {

		BackupOrderValues = gcnew array <String ^>(subvalues->Length);

		for (int i = 0; i < subvalues->Length; i++) {
			try {
				if ((obj = BOKey->GetValue(subvalues[i])) == nullptr)
					throw;
				BackupOrderValues[i] = obj->ToString();
			}
			catch (Exception ^e) {
				//rzbkey->SetValue("Locality.BackupOrder", "2, 3, 4");
				//BackupOrderValue[i] = "2, 3, 4";
				MessageBox::Show("Could not get Backup Order values.");
			}
		}
	}

	
	try {
		if ((obj = rzbkey->GetValue("Cache.BadLimit")) == nullptr)
			throw;
		BadLimitValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Cache.BadLimit", 256);
		BadLimitValue = "256";
	}

	try {
		if ((obj = rzbkey->GetValue("Cache.GoodLimit")) == nullptr)
			throw;
		GoodLimitValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Cache.GoodLimit", 256);
		GoodLimitValue = "256";
	}

	try {
		if ((obj = rzbkey->GetValue("MessageQueue.Host")) == nullptr)
			throw;
		HostValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("MessageQueue.Host", "localhost");
		HostValue = "localhost";
	}

	try {
		if ((obj = rzbkey->GetValue("MessageQueue.Port")) == nullptr)
			throw;
		PortValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("MessageQueue.Port", 61612);
		PortValue = "61612";
	}

	try {
		if ((obj = rzbkey->GetValue("MessageQueue.Username")) == nullptr)
			throw;
		UsernameValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("MessageQueue.Username", "User");
		UsernameValue = "User";
	}

	try {
		if ((obj = rzbkey->GetValue("MessageQueue.Password")) == nullptr)
			throw;
		PasswordValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("MessageQueue.Password", "Password");
		PasswordValue = "Password";
	}

	try {
		if ((obj = rzbkey->GetValue("MessageQueue.SSL")) == nullptr)
			throw;
		SSLValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("MessageQueue.SSL", "False");
		SSLValue = "False";
	}

	try {
		if ((obj = rzbkey->GetValue("Log.Destination")) == nullptr)
			throw;
		DestinationValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Log.Destination", "stderr");
		DestinationValue = "stderr";
	}

	try {
		if ((obj = rzbkey->GetValue("Log.Level")) == nullptr)
			throw;
		LogLevelValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Log.Level", "debug");
		LogLevelValue = "debug";
	}
	
	try {
		if ((obj = rzbkey->GetValue("Log.Syslog_Facility")) == nullptr)
			throw;
		SyslogFacilityValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Log.Syslog_Facility", "daemon");
		SyslogFacilityValue = "daemon";
	}

	try {
		if ((obj = rzbkey->GetValue("Log.File")) == nullptr)
			throw;
		FilePathValue = obj->ToString();
	}
	catch (Exception ^e) {
		rzbkey->SetValue("Log.File", "");
		FilePathValue = "";
	}

}

void RZBConfiguration::initGlobalGroup() {
	       /*
	        MaxBlockLabel = gcnew System::Windows::Forms::Label();
			MaxBlockTextBox = gcnew System::Windows::Forms::TextBox();
			MaxThreadsLabel = gcnew System::Windows::Forms::Label();
	        MaxThreadsTextBox = gcnew System::Windows::Forms::TextBox();
			DeadLabel = gcnew System::Windows::Forms::Label();
			HelloLabel = gcnew System::Windows::Forms::Label();
			DeadTextBox = gcnew System::Windows::Forms::TextBox();
			HelloTextBox = gcnew System::Windows::Forms::TextBox();
			SHA512Radio = gcnew System::Windows::Forms::RadioButton();
			SHA256Radio = gcnew System::Windows::Forms::RadioButton();
			SHA224Radio = gcnew System::Windows::Forms::RadioButton();
			HashTypeLabel = gcnew System::Windows::Forms::Label();
			SHA1Radio = gcnew System::Windows::Forms::RadioButton();
			MD5Radio = gcnew System::Windows::Forms::RadioButton();
			MsgFormatLabel = gcnew System::Windows::Forms::Label();
			JSONRadio = gcnew System::Windows::Forms::RadioButton();
			BinaryRadio = gcnew System::Windows::Forms::RadioButton();
			HashTypePanel = gcnew System::Windows::Forms::Panel();
			MsgFormatPanel = gcnew System::Windows::Forms::Panel();
			GlobalGroupBox = (gcnew System::Windows::Forms::GroupBox());
			*/
			// 
			// SHA512Radio
			// 
			SHA512Radio->AutoSize = true;
			SHA512Radio->Location = System::Drawing::Point(83, 125);
			SHA512Radio->Name = L"SHA512Radio";
			SHA512Radio->Size = System::Drawing::Size(68, 17);
			SHA512Radio->TabIndex = 18;
			SHA512Radio->TabStop = true;
			SHA512Radio->Text = L"SHA-512";
			SHA512Radio->UseVisualStyleBackColor = true;
			// 
			// SHA256Radio
			// 
			SHA256Radio->AutoSize = true;
			SHA256Radio->Location = System::Drawing::Point(83, 102);
			SHA256Radio->Name = L"SHA256Radio";
			SHA256Radio->Size = System::Drawing::Size(68, 17);
			SHA256Radio->TabIndex = 17;
			SHA256Radio->TabStop = true;
			SHA256Radio->Text = L"SHA-256";
			SHA256Radio->UseVisualStyleBackColor = true;
			// 
			// SHA224Radio
			// 
			SHA224Radio->AutoSize = true;
			SHA224Radio->Location = System::Drawing::Point(83, 79);
			SHA224Radio->Name = L"SHA224Radio";
			SHA224Radio->Size = System::Drawing::Size(68, 17);
			SHA224Radio->TabIndex = 16;
			SHA224Radio->TabStop = true;
			SHA224Radio->Text = L"SHA-224";
			SHA224Radio->UseVisualStyleBackColor = true;
			// 
			// HashTypeLabel
			// 
			HashTypeLabel->AutoSize = true;
			HashTypeLabel->Location = System::Drawing::Point(18, 13);
			HashTypeLabel->Name = L"HashTypeLabel";
			HashTypeLabel->Size = System::Drawing::Size(62, 13);
			HashTypeLabel->TabIndex = 15;
			HashTypeLabel->Text = L"Hash Type:";
			// 
			// SHA1Radio
			// 
			SHA1Radio->AutoSize = true;
			SHA1Radio->Location = System::Drawing::Point(83, 56);
			SHA1Radio->Name = L"SHA1Radio";
			SHA1Radio->Size = System::Drawing::Size(56, 17);
			SHA1Radio->TabIndex = 14;
			SHA1Radio->TabStop = true;
			SHA1Radio->Text = L"SHA-1";
			SHA1Radio->UseVisualStyleBackColor = true;
			// 
			// MD5Radio
			// 
			MD5Radio->AutoSize = true;
			MD5Radio->Location = System::Drawing::Point(83, 33);
			MD5Radio->Name = L"MD5Radio";
			MD5Radio->Size = System::Drawing::Size(48, 17);
			MD5Radio->TabIndex = 13;
			MD5Radio->TabStop = true;
			MD5Radio->Text = L"MD5";
			MD5Radio->UseVisualStyleBackColor = true;
			// 
			// HashTypePanel
			// 
			HashTypePanel->Controls->Add(SHA512Radio);
			HashTypePanel->Controls->Add(SHA256Radio);
			HashTypePanel->Controls->Add(SHA224Radio);
			HashTypePanel->Controls->Add(HashTypeLabel);
			HashTypePanel->Controls->Add(SHA1Radio);
			HashTypePanel->Controls->Add(MD5Radio);
			HashTypePanel->Location = System::Drawing::Point(286, 124);
			HashTypePanel->Name = L"HashTypePanel";
			HashTypePanel->Size = System::Drawing::Size(182, 176);
			HashTypePanel->TabIndex = 20;
			// 
			// JSONRadio
			// 
			JSONRadio->AutoSize = true;
			JSONRadio->Location = System::Drawing::Point(127, 57);
			JSONRadio->Name = L"JSONRadio";
			JSONRadio->Size = System::Drawing::Size(53, 17);
			JSONRadio->TabIndex = 11;
			JSONRadio->TabStop = true;
			JSONRadio->Text = L"JSON";
			JSONRadio->UseVisualStyleBackColor = true;
			// 
			// BinaryRadio
			// 
			BinaryRadio->AutoSize = true;
			BinaryRadio->Location = System::Drawing::Point(127, 34);
			BinaryRadio->Name = L"BinaryRadio";
			BinaryRadio->Size = System::Drawing::Size(54, 17);
			BinaryRadio->TabIndex = 10;
			BinaryRadio->TabStop = true;
			BinaryRadio->Text = L"Binary";
			BinaryRadio->UseVisualStyleBackColor = true;
			// 
			// MsgFormatPanel
			// 
			MsgFormatPanel->Controls->Add(MsgFormatLabel);
			MsgFormatPanel->Controls->Add(JSONRadio);
			MsgFormatPanel->Controls->Add(BinaryRadio);
			MsgFormatPanel->Location = System::Drawing::Point(34, 123);
			MsgFormatPanel->Name = L"MsgFormatPanel";
			MsgFormatPanel->Size = System::Drawing::Size(226, 96);
			MsgFormatPanel->TabIndex = 19;
			// 
			// DeadLabel
			// 
			DeadLabel->AutoSize = true;
			DeadLabel->Location = System::Drawing::Point(304, 75);
			DeadLabel->Name = L"DeadLabel";
			DeadLabel->Size = System::Drawing::Size(62, 13);
			DeadLabel->TabIndex = 9;
			DeadLabel->Text = L"Dead Time:";
			// 
			// HelloLabel
			// 
			HelloLabel->AutoSize = true;
			HelloLabel->Location = System::Drawing::Point(304, 49);
			HelloLabel->Name = L"HelloLabel";
			HelloLabel->Size = System::Drawing::Size(60, 13);
			HelloLabel->TabIndex = 8;
			HelloLabel->Text = L"Hello Time:";
			// 
			// DeadTextBox
			// 
			DeadTextBox->Location = System::Drawing::Point(369, 72);
			DeadTextBox->Name = L"DeadTextBox";
			DeadTextBox->Size = System::Drawing::Size(100, 20);
			DeadTextBox->TabIndex = 7;
			// 
			// HelloTextBox
			// 
			HelloTextBox->Location = System::Drawing::Point(369, 46);
			HelloTextBox->Name = L"HelloTextBox";
			HelloTextBox->Size = System::Drawing::Size(100, 20);
			HelloTextBox->TabIndex = 6;
	        // 
			// MaxBlockLabel
			// 
			MaxBlockLabel->AutoSize = true;
			MaxBlockLabel->Location = System::Drawing::Point(45, 75);
			MaxBlockLabel->Name = L"MaxBlockLabel";
			MaxBlockLabel->Size = System::Drawing::Size(110, 13);
			MaxBlockLabel->TabIndex = 3;
			MaxBlockLabel->Text = L"Maximum Block Size: ";
			// 
			// MaxBlockTextBox
			// 
			MaxBlockTextBox->Location = System::Drawing::Point(161, 72);
			MaxBlockTextBox->Name = L"MaxBlockTextBox";
			MaxBlockTextBox->Size = System::Drawing::Size(100, 20);
			MaxBlockTextBox->TabIndex = 2;
			// 
			// MaxThreadsLabel
			// 
			MaxThreadsLabel->AutoSize = true;
			MaxThreadsLabel->Location = System::Drawing::Point(45, 49);
			MaxThreadsLabel->Name = L"MaxThreadsLabel";
			MaxThreadsLabel->Size = System::Drawing::Size(96, 13);
			MaxThreadsLabel->TabIndex = 1;
			MaxThreadsLabel->Text = L"Maximum Threads:";
			// 
			// MaxThreadsTextBox
			// 
			MaxThreadsTextBox->Location = System::Drawing::Point(161, 46);
			MaxThreadsTextBox->Name = L"MaxThreadsTextBox";
			MaxThreadsTextBox->Size = System::Drawing::Size(100, 20);
			MaxThreadsTextBox->TabIndex = 0;
			//
			// TransferPasswordLabel
			//
			TransferPasswordLabel->AutoSize = true;
			TransferPasswordLabel->Location = System::Drawing::Point(45, 101);
			TransferPasswordLabel->Name = L"TransferPasswordLabel";
			TransferPasswordLabel->Size = System::Drawing::Size(96, 13);
			TransferPasswordLabel->TabIndex = 22;
			TransferPasswordLabel->Text = L"TransferPassword:";
			//
			// TransferPasswordTextBox
			//
			TransferPasswordTextBox->Location = System::Drawing::Point(161, 98);
			TransferPasswordTextBox->Name = L"TransferPasswordTextBox";
			TransferPasswordTextBox->Size = System::Drawing::Size(100, 20);
			TransferPasswordTextBox->TabIndex = 21; 
	        // 
			// GlobalGroupBox
			// 
			GlobalGroupBox->Controls->Add(HashTypePanel);
			GlobalGroupBox->Controls->Add(MsgFormatPanel);
			GlobalGroupBox->Controls->Add(DeadLabel);
			GlobalGroupBox->Controls->Add(HelloLabel);
			GlobalGroupBox->Controls->Add(DeadTextBox);
			GlobalGroupBox->Controls->Add(HelloTextBox);
			GlobalGroupBox->Controls->Add(MaxBlockLabel);
			GlobalGroupBox->Controls->Add(MaxBlockTextBox);
			GlobalGroupBox->Controls->Add(MaxThreadsLabel);
			GlobalGroupBox->Controls->Add(MaxThreadsTextBox);
			GlobalGroupBox->Controls->Add(TransferPasswordLabel);
			GlobalGroupBox->Controls->Add(TransferPasswordTextBox);
			GlobalGroupBox->Location = System::Drawing::Point(239, 4);
			GlobalGroupBox->Name = L"GlobalGroupBox";
			GlobalGroupBox->Size = System::Drawing::Size(645, 499);
			GlobalGroupBox->TabIndex = 1;
			GlobalGroupBox->TabStop = false;
			GlobalGroupBox->Text = L"Global";
			GlobalGroupBox->Visible = false;

			populateGlobalGroup();

			GlobalGroupBox->ResumeLayout();
			GlobalGroupBox->PerformLayout();

}

void RZBConfiguration::initCacheGroup() {
			// 
			// MaxGoodLabel
			// 
			MaxGoodLabel->AutoSize = true;
			MaxGoodLabel->Location = System::Drawing::Point(34, 74);
			MaxGoodLabel->Name = L"MaxGoodLabel";
			MaxGoodLabel->Size = System::Drawing::Size(118, 13);
			MaxGoodLabel->TabIndex = 3;
			MaxGoodLabel->Text = L"Maximum Good Entries:";
			// 
			// MaxBadLabel
			// 
			MaxBadLabel->AutoSize = true;
			MaxBadLabel->Location = System::Drawing::Point(34, 48);
			MaxBadLabel->Name = L"MaxBadLabel";
			MaxBadLabel->Size = System::Drawing::Size(111, 13);
			MaxBadLabel->TabIndex = 2;
			MaxBadLabel->Text = L"Maximum Bad Entries:";
			// 
			// MaxGoodTextBox
			// 
			MaxGoodTextBox->Location = System::Drawing::Point(158, 71);
			MaxGoodTextBox->Name = L"MaxGoodTextBox";
			MaxGoodTextBox->Size = System::Drawing::Size(100, 20);
			MaxGoodTextBox->TabIndex = 1;
			// 
			// MaxBadTextBox
			// 
			MaxBadTextBox->Location = System::Drawing::Point(158, 45);
			MaxBadTextBox->Name = L"MaxBadTextBox";
			MaxBadTextBox->Size = System::Drawing::Size(100, 20);
			MaxBadTextBox->TabIndex = 0;
			// 
			// CacheGroupBox
			// 
			CacheGroupBox->Controls->Add(MaxGoodLabel);
			CacheGroupBox->Controls->Add(MaxBadLabel);
			CacheGroupBox->Controls->Add(MaxGoodTextBox);
			CacheGroupBox->Controls->Add(MaxBadTextBox);
			CacheGroupBox->Location = System::Drawing::Point(239, 4);
			CacheGroupBox->Name = L"CacheGroupBox";
			CacheGroupBox->Size = System::Drawing::Size(648, 499);
			CacheGroupBox->TabIndex = 6;
			CacheGroupBox->TabStop = false;
			CacheGroupBox->Text = L"Local Cache";
			CacheGroupBox->Visible = false;

            populateCacheGroup();

}

void RZBConfiguration::initLocalGroup() {
						// 
			// BackupOrderLabel
			// 
			BackupOrderLabel->AutoSize = true;
			BackupOrderLabel->Location = System::Drawing::Point(31, 62);
			BackupOrderLabel->Name = L"BackupOrderLabel";
			BackupOrderLabel->Size = System::Drawing::Size(79, 13);
			BackupOrderLabel->TabIndex = 5;
			BackupOrderLabel->Text = L"Back-up Order:";
			// 
			// StoragePathLabel
			// 
			StoragePathLabel->AutoSize = true;
			StoragePathLabel->Location = System::Drawing::Point(31, 88);
			StoragePathLabel->Name = L"StoragePathLabel";
			StoragePathLabel->Size = System::Drawing::Size(102, 13);
			StoragePathLabel->TabIndex = 4;
			StoragePathLabel->Text = L"Block Storage Path:";
			// 
			// StoragePathTextBox
			// 
			StoragePathTextBox->Location = System::Drawing::Point(139, 85);
			StoragePathTextBox->Name = L"StoragePathTextBox";
			StoragePathTextBox->Size = System::Drawing::Size(224, 20);
			StoragePathTextBox->TabIndex = 3;
			// 
			// BackupOrderTextBox
			// 
			BackupOrderTextBox->Location = System::Drawing::Point(114, 59);
			BackupOrderTextBox->Name = L"BackupOrderTextBox";
			BackupOrderTextBox->Size = System::Drawing::Size(100, 20);
			BackupOrderTextBox->TabIndex = 2;
			// 
			// LocalityIDLabel
			// 
			LocalityIDLabel->AutoSize = true;
			LocalityIDLabel->Location = System::Drawing::Point(31, 36);
			LocalityIDLabel->Name = L"LocalityIDLabel";
			LocalityIDLabel->Size = System::Drawing::Size(63, 13);
			LocalityIDLabel->TabIndex = 1;
			LocalityIDLabel->Text = L"Locality ID: ";
			// 
			// LocalityIDTextBox
			// 
			LocalityIDTextBox->Location = System::Drawing::Point(114, 33);
			LocalityIDTextBox->Name = L"LocalityIDTextBox";
			LocalityIDTextBox->Size = System::Drawing::Size(100, 20);
			LocalityIDTextBox->TabIndex = 0;
	
	        // 
			// LocalityGroupBox
			// 
			LocalityGroupBox->Controls->Add(BackupOrderLabel);
			LocalityGroupBox->Controls->Add(StoragePathLabel);
			LocalityGroupBox->Controls->Add(StoragePathTextBox);
			LocalityGroupBox->Controls->Add(BackupOrderTextBox);
			LocalityGroupBox->Controls->Add(LocalityIDLabel);
			LocalityGroupBox->Controls->Add(LocalityIDTextBox);
			LocalityGroupBox->Location = System::Drawing::Point(239, 4);
			LocalityGroupBox->Name = L"LocalityGroupBox";
			LocalityGroupBox->Size = System::Drawing::Size(648, 499);
			LocalityGroupBox->TabIndex = 19;
			LocalityGroupBox->TabStop = false;
			LocalityGroupBox->Text = L"Locality";
			LocalityGroupBox->Visible = false;

			populateLocalGroup();

}

void RZBConfiguration::initMQGroup() {
			// 
			// SSLDisabledRadio
			// 
			SSLDisabledRadio->AutoSize = true;
			SSLDisabledRadio->Location = System::Drawing::Point(43, 41);
			SSLDisabledRadio->Name = L"SSLDisabledRadio";
			SSLDisabledRadio->Size = System::Drawing::Size(66, 17);
			SSLDisabledRadio->TabIndex = 13;
			SSLDisabledRadio->TabStop = true;
			SSLDisabledRadio->Text = L"Disabled";
			SSLDisabledRadio->UseVisualStyleBackColor = true;
			// 
			// SSLEnabledRadio
			// 
			SSLEnabledRadio->AutoSize = true;
			SSLEnabledRadio->Location = System::Drawing::Point(43, 19);
			SSLEnabledRadio->Name = L"SSLEnabledRadio";
			SSLEnabledRadio->Size = System::Drawing::Size(64, 17);
			SSLEnabledRadio->TabIndex = 12;
			SSLEnabledRadio->TabStop = true;
			SSLEnabledRadio->Text = L"Enabled";
			SSLEnabledRadio->UseVisualStyleBackColor = true;
			// 
			// SSLLabel
			// 
			SSLLabel->AutoSize = true;
			SSLLabel->Location = System::Drawing::Point(4, 6);
			SSLLabel->Name = L"SSLLabel";
			SSLLabel->Size = System::Drawing::Size(30, 13);
			SSLLabel->TabIndex = 7;
			SSLLabel->Text = L"SSL:";
				// 
			// SSLPanel
			// 
			SSLPanel->Controls->Add(SSLDisabledRadio);
			SSLPanel->Controls->Add(SSLEnabledRadio);
			SSLPanel->Controls->Add(SSLLabel);
			SSLPanel->Location = System::Drawing::Point(264, 29);
			SSLPanel->Name = L"SSLPanel";
			SSLPanel->Size = System::Drawing::Size(152, 84);
			SSLPanel->TabIndex = 15;
			// 
			// UsernameTextBox
			// 
			UsernameTextBox->Location = System::Drawing::Point(95, 84);
			UsernameTextBox->Name = L"UsernameTextBox";
			UsernameTextBox->Size = System::Drawing::Size(100, 20);
			UsernameTextBox->TabIndex = 11;
			// 
			// PasswordTextBox
			// 
			PasswordTextBox->Location = System::Drawing::Point(95, 110);
			PasswordTextBox->Name = L"PasswordTextBox";
			PasswordTextBox->Size = System::Drawing::Size(100, 20);
			PasswordTextBox->TabIndex = 10;

			// 
			// PasswordLabel
			// 
			PasswordLabel->AutoSize = true;
			PasswordLabel->Location = System::Drawing::Point(31, 113);
			PasswordLabel->Name = L"PasswordLabel";
			PasswordLabel->Size = System::Drawing::Size(56, 13);
			PasswordLabel->TabIndex = 6;
			PasswordLabel->Text = L"Password:";
			// 
			// UsernameLabel
			// 
			UsernameLabel->AutoSize = true;
			UsernameLabel->Location = System::Drawing::Point(31, 87);
			UsernameLabel->Name = L"UsernameLabel";
			UsernameLabel->Size = System::Drawing::Size(58, 13);
			UsernameLabel->TabIndex = 5;
			UsernameLabel->Text = L"Username:";
			// 
			// PortLabel
			// 
			PortLabel->AutoSize = true;
			PortLabel->Location = System::Drawing::Point(31, 61);
			PortLabel->Name = L"PortLabel";
			PortLabel->Size = System::Drawing::Size(29, 13);
			PortLabel->TabIndex = 4;
			PortLabel->Text = L"Port:";
			// 
			// HostLabel
			// 
			HostLabel->AutoSize = true;
			HostLabel->Location = System::Drawing::Point(31, 35);
			HostLabel->Name = L"HostLabel";
			HostLabel->Size = System::Drawing::Size(32, 13);
			HostLabel->TabIndex = 3;
			HostLabel->Text = L"Host:";			
			// 
			// PortTextBox
			// 
			PortTextBox->Location = System::Drawing::Point(72, 58);
			PortTextBox->Name = L"PortTextBox";
			PortTextBox->Size = System::Drawing::Size(100, 20);
			PortTextBox->TabIndex = 9;
			// 
			// HostTextBox
			// 
			HostTextBox->Location = System::Drawing::Point(72, 32);
			HostTextBox->Name = L"HostTextBox";
			HostTextBox->Size = System::Drawing::Size(100, 20);
			HostTextBox->TabIndex = 8;		
			// 
			// MessageQueueGroupBox
			// 
			MessageQueueGroupBox->Controls->Add(SSLPanel);
			MessageQueueGroupBox->Controls->Add(UsernameTextBox);
			MessageQueueGroupBox->Controls->Add(PasswordTextBox);
			MessageQueueGroupBox->Controls->Add(PortTextBox);
			MessageQueueGroupBox->Controls->Add(HostTextBox);
			MessageQueueGroupBox->Controls->Add(PasswordLabel);
			MessageQueueGroupBox->Controls->Add(UsernameLabel);
			MessageQueueGroupBox->Controls->Add(PortLabel);
			MessageQueueGroupBox->Controls->Add(HostLabel);
			MessageQueueGroupBox->Location = System::Drawing::Point(239, 4);
			MessageQueueGroupBox->Name = L"MessageQueueGroupBox";
			MessageQueueGroupBox->Size = System::Drawing::Size(648, 499);
			MessageQueueGroupBox->TabIndex = 4;
			MessageQueueGroupBox->TabStop = false;
			MessageQueueGroupBox->Text = L"Message Queue";
			MessageQueueGroupBox->Visible = false;

			populateMQGroup();

}

void RZBConfiguration::initLogGroup() {
				// 
			// LogFilePathLabel
			// 
			LogFilePathLabel->AutoSize = true;
			LogFilePathLabel->Location = System::Drawing::Point(45, 113);
			LogFilePathLabel->Name = L"LogFilePathLabel";
			LogFilePathLabel->Size = System::Drawing::Size(72, 13);
			LogFilePathLabel->TabIndex = 7;
			LogFilePathLabel->Text = L"Log File Path:";
			// 
			// SyslogFacilityLabel
			// 
			SyslogFacilityLabel->AutoSize = true;
			SyslogFacilityLabel->Location = System::Drawing::Point(45, 59);
			SyslogFacilityLabel->Name = L"SyslogFacilityLabel";
			SyslogFacilityLabel->Size = System::Drawing::Size(76, 13);
			SyslogFacilityLabel->TabIndex = 6;
			SyslogFacilityLabel->Text = L"Syslog Facility:";
			// 
			// LogLevelLabel
			// 
			LogLevelLabel->AutoSize = true;
			LogLevelLabel->Location = System::Drawing::Point(45, 35);
			LogLevelLabel->Name = L"LogLevelLabel";
			LogLevelLabel->Size = System::Drawing::Size(57, 13);
			LogLevelLabel->TabIndex = 5;
			LogLevelLabel->Text = L"Log Level:";
			// 
			// DestinationLabel
			// 
			DestinationLabel->AutoSize = true;
			DestinationLabel->Location = System::Drawing::Point(278, 35);
			DestinationLabel->Name = L"DestinationLabel";
			DestinationLabel->Size = System::Drawing::Size(63, 13);
			DestinationLabel->TabIndex = 4;
			DestinationLabel->Text = L"Destination:";
			// 
			// LogFilePathTextBox
			// 
			LogFilePathTextBox->Location = System::Drawing::Point(128, 110);
			LogFilePathTextBox->Name = L"LogFilePathTextBox";
			LogFilePathTextBox->Size = System::Drawing::Size(185, 20);
			LogFilePathTextBox->TabIndex = 3;
			// 
			// LogLevelComboBox
			// 
			LogLevelComboBox->FormattingEnabled = true;
			LogLevelComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(8) {L"Emergency", L"Alert", L"Critical", L"Error", 
				L"Warning", L"Notice", L"Info", L"Debug"});
			LogLevelComboBox->Location = System::Drawing::Point(128, 32);
			LogLevelComboBox->Name = L"LogLevelComboBox";
			LogLevelComboBox->Size = System::Drawing::Size(121, 21);
			LogLevelComboBox->TabIndex = 2;
			// 
			// SyslogFacilityComboBox
			// 
			SyslogFacilityComboBox->FormattingEnabled = true;
			SyslogFacilityComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(10) {L"daemon", L"user", L"local0", L"local1", 
				L"local2", L"local3", L"local4", L"local5", L"local6", L"local7"});
			SyslogFacilityComboBox->Location = System::Drawing::Point(128, 58);
			SyslogFacilityComboBox->Name = L"SyslogFacilityComboBox";
			SyslogFacilityComboBox->Size = System::Drawing::Size(121, 21);
			SyslogFacilityComboBox->TabIndex = 1;
			// 
			// DestinationComboBox
			// 
			DestinationComboBox->FormattingEnabled = true;
			DestinationComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"syslog", L"stderr", L"Log file"});
			DestinationComboBox->Location = System::Drawing::Point(361, 32);
			DestinationComboBox->Name = L"DestinationComboBox";
			DestinationComboBox->Size = System::Drawing::Size(121, 21);
			DestinationComboBox->TabIndex = 0;
			// 
			// LoggingGroupBox
			// 
			LoggingGroupBox->Controls->Add(LogFilePathLabel);
			LoggingGroupBox->Controls->Add(SyslogFacilityLabel);
			LoggingGroupBox->Controls->Add(LogLevelLabel);
			LoggingGroupBox->Controls->Add(DestinationLabel);
			LoggingGroupBox->Controls->Add(LogFilePathTextBox);
			LoggingGroupBox->Controls->Add(LogLevelComboBox);
			LoggingGroupBox->Controls->Add(SyslogFacilityComboBox);
			LoggingGroupBox->Controls->Add(DestinationComboBox);
			LoggingGroupBox->Location = System::Drawing::Point(239, 4);
			LoggingGroupBox->Name = L"LoggingGroupBox";
			LoggingGroupBox->Size = System::Drawing::Size(648, 499);
			LoggingGroupBox->TabIndex = 14;
			LoggingGroupBox->TabStop = false;
			LoggingGroupBox->Text = L"Logging";
			LoggingGroupBox->Visible = false;

			populateLogGroup();
}

void RZBConfiguration::populateGlobalGroup() {
			 MaxThreadsTextBox->Text = MaxThreadsValue;
			 MaxBlockTextBox->Text = MaxBlockSizeValue;
			 HelloTextBox->Text = HelloTimeValue;
			 DeadTextBox->Text = DeadTimeValue;
			 TransferPasswordTextBox->Text = TransferPasswordValue;

			 if (String::Compare(MessageFormatValue, "binary", true) == 0) {
				 BinaryRadio->Checked = true;
			 }
			 else if (String::Compare(MessageFormatValue, "JSON", true) == 0) {
				 JSONRadio->Checked = true;
			 }
			 else {
				 MessageBox::Show("Invalid Global.MessageFormat registry value %S", MessageFormatValue);
				 //Error
			 }

			 if (String::Compare(HashTypeValue, "MD5", true) == 0) {
				 MD5Radio->Checked = true;
			 }
			 else if (String::Compare(HashTypeValue, "SHA1", true) == 0) {
				 SHA1Radio->Checked = true;
			 }
			 else if (String::Compare(HashTypeValue, "SHA224", true) == 0) {
				 SHA224Radio->Checked = true;
			 }
			 else if (String::Compare(HashTypeValue, "SHA256", true) == 0) {
				 SHA256Radio->Checked = true;
			 }
			 else if (String::Compare(HashTypeValue, "SHA512", true) == 0) {
				 SHA512Radio->Checked = true;
			 }
			 else {
				 MessageBox::Show("Invalid Global.HashType registry value %S", HashTypeValue);
				 //Error
			 }
}

	void RZBConfiguration::populateCacheGroup(){
			MaxGoodTextBox->Text = GoodLimitValue;
			MaxBadTextBox->Text = BadLimitValue;
	}
	void RZBConfiguration::populateLocalGroup(){
			LocalityIDTextBox->Text = LocalityIDValue;
			BackupOrderTextBox->Text = "";
			for (int i = 0; i < BackupOrderValues->Length - 1; i++) {
				BackupOrderTextBox->Text = String::Concat(BackupOrderTextBox->Text, BackupOrderValues[i], ", ");
			}
			BackupOrderTextBox->Text = String::Concat(BackupOrderTextBox->Text, BackupOrderValues[BackupOrderValues->Length-1]);
			StoragePathTextBox->Text = BlockStoreValue;
	}
	void RZBConfiguration::populateMQGroup(){
			HostTextBox->Text = HostValue;
			PortTextBox->Text = PortValue;
			UsernameTextBox->Text = UsernameValue;
			PasswordTextBox->Text = PasswordValue;

			if (String::Compare(SSLValue, "False", true) == 0) {
				SSLDisabledRadio->Checked = true;
			}
			else if (String::Compare(SSLValue, "True", true) == 0) {
				SSLEnabledRadio->Checked = true;
			}
			else {
				//Error
				//Reset to default
			}
	}
	void RZBConfiguration::populateLogGroup(){
			if (String::Compare(LogLevelValue, "emergency", true) == 0) {
				LogLevelComboBox->SelectedIndex = 0;
			}
			else if (String::Compare(LogLevelValue, "alert", true) == 0) {
				LogLevelComboBox->SelectedIndex = 1;
			}
			else if (String::Compare(LogLevelValue, "critical", true) == 0) {
				LogLevelComboBox->SelectedIndex = 2;
			}
			else if (String::Compare(LogLevelValue, "error", true) == 0) {
				LogLevelComboBox->SelectedIndex = 3;
			}
			else if (String::Compare(LogLevelValue, "warning", true) == 0) {
				LogLevelComboBox->SelectedIndex = 4;
			}
			else if (String::Compare(LogLevelValue, "notice", true) == 0) {
				LogLevelComboBox->SelectedIndex = 5;
			}
			else if (String::Compare(LogLevelValue, "info", true) == 0) {
				LogLevelComboBox->SelectedIndex = 6;
			}
			else if (String::Compare(LogLevelValue, "debug", true) == 0) {
				LogLevelComboBox->SelectedIndex = 7;
			}
			else {
				//Error
				//Reset to default
			}

			if (String::Compare(SyslogFacilityValue, "daemon", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 0;
			}
			else if (String::Compare(SyslogFacilityValue, "user", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 1;
			}
			else if (String::Compare(SyslogFacilityValue, "local0", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 2;
			}
			else if (String::Compare(SyslogFacilityValue, "local1", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 3;
			}
			else if (String::Compare(SyslogFacilityValue, "local2", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 4;
			}
			else if (String::Compare(SyslogFacilityValue, "local3", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 5;
			}
			else if (String::Compare(SyslogFacilityValue, "local4", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 6;
			}
			else if (String::Compare(SyslogFacilityValue, "local5", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 7;
			}
			else if (String::Compare(SyslogFacilityValue, "local6", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 8;
			}
			else if (String::Compare(SyslogFacilityValue, "local7", true) == 0) {
				SyslogFacilityComboBox->SelectedIndex = 9;
			}
			else {
			}

			if (String::Compare(DestinationValue, "syslog", true) == 0) {
				DestinationComboBox->SelectedIndex = 0;
			}
			else if (String::Compare(DestinationValue, "stderr", true) == 0) {
				DestinationComboBox->SelectedIndex = 1;
			}
			else if (String::Compare(DestinationValue, "file", true) == 0) {
				DestinationComboBox->SelectedIndex = 2;
			}
			else {
				//Error
				//Reset to default
			}

			LogFilePathTextBox->Text = FilePathValue;
	}

	void RZBConfiguration::storeAll() {
		
		//Values to convert from int
		
		rzbkey->SetValue("Global.MaxThreads", Convert::ToInt32(MaxThreadsTextBox->Text, 10));
		rzbkey->SetValue("Global.MaxBlockSize", Convert::ToInt32(MaxBlockTextBox->Text, 10));
		rzbkey->SetValue("Global.HelloTime", Convert::ToInt32(HelloTextBox->Text, 10));	
		rzbkey->SetValue("Global.DeadTime", Convert::ToInt32(DeadTextBox->Text, 10));	
		rzbkey->SetValue("Locality.Id", Convert::ToInt32(LocalityIDTextBox->Text, 10));		
		rzbkey->SetValue("Cache.BadLimit", Convert::ToInt32(MaxBadTextBox->Text, 10));	
		rzbkey->SetValue("Cache.GoodLimit", Convert::ToInt32(MaxGoodTextBox->Text, 10));	
		rzbkey->SetValue("MessageQueue.Port", Convert::ToInt32(PortTextBox->Text, 10));	

		//Values to convert from list or radio
		if (SSLDisabledRadio->Checked == true)
			rzbkey->SetValue("MessageQueue.SSL", "False");
		else
			rzbkey->SetValue("MessageQueue.SSL", "True");

		switch (SyslogFacilityComboBox->SelectedIndex) {
			case 0:
				rzbkey->SetValue("Log.Syslog_Facility", "daemon");
				break;

			case 1:
				rzbkey->SetValue("Log.Syslog_Facility", "user");
				break;

			case 2:
				rzbkey->SetValue("Log.Syslog_Facility", "local0");
				break;

			case 3:
				rzbkey->SetValue("Log.Syslog_Facility", "local1");
				break;

			case 4:
				rzbkey->SetValue("Log.Syslog_Facility", "local2");
				break;

			case 5:
				rzbkey->SetValue("Log.Syslog_Facility", "local3");
				break;

			case 6:
				rzbkey->SetValue("Log.Syslog_Facility", "local4");
				break;

			case 7:
				rzbkey->SetValue("Log.Syslog_Facility", "local5");
				break;

			case 8:
				rzbkey->SetValue("Log.Syslog_Facility", "local6");
				break;

			case 9:
				rzbkey->SetValue("Log.Syslog_Facility", "local7");
				break;

			default:
				//Error
				MessageBox::Show("If you get here, I'll give you a cookie.");
		}

		switch (DestinationComboBox->SelectedIndex) {
			case 0:
				rzbkey->SetValue("Log.Destination", "syslog");
				break;

			case 1:
				rzbkey->SetValue("Log.Destination", "stderr");
				break;

			case 2:
				rzbkey->SetValue("Log.Destination", "file");
				break;

			default:
				//Error
				MessageBox::Show("If you get here, I'll give you a cookie.");
		}

		switch (LogLevelComboBox->SelectedIndex) {
			case 0:
				rzbkey->SetValue("Log.Level", "emergency");
				break;

			case 1:
				rzbkey->SetValue("Log.Level", "alert");
				break;

			case 2:
				rzbkey->SetValue("Log.Level", "critical");
				break;

			case 3:
				rzbkey->SetValue("Log.Level", "error");
				break;

			case 4:
				rzbkey->SetValue("Log.Level", "warning");
				break;

			case 5:
				rzbkey->SetValue("Log.Level", "notice");
				break;

			case 6:
				rzbkey->SetValue("Log.Level", "info");
				break;
			case 7:
				rzbkey->SetValue("Log.Level", "debug");
				break;

			default:
				//Error
				MessageBox::Show("If you get here, I'll give you a cookie.");
		}

			


		if (MD5Radio->Checked == true)
			rzbkey->SetValue("Global.HashType", "MD5");
		else if (SHA1Radio->Checked == true)
			rzbkey->SetValue("Global.HashType", "SHA1");
		else if (SHA224Radio->Checked == true)
			rzbkey->SetValue("Global.HashType", "SHA224");
		else if (SHA256Radio->Checked == true)
			rzbkey->SetValue("Global.HashType", "SHA256");
		else {
			rzbkey->SetValue("Global.HashType", "SHA512");
		}

		if (BinaryRadio->Checked == true)
			rzbkey->SetValue("Global.MessageFormat", "binary");	
		else {
			rzbkey->SetValue("Global.MessageFormat", "JSON");
		}

		//Values that are subkeys
		RegistryKey ^BOKey = rzbkey->OpenSubKey("Locality.BackupOrder", true);
		for (int i = 0; i < subvalues->Length; i++) {
			BOKey->DeleteValue(subvalues[i]);
		}

		array <Char>^ sep = gcnew array<Char> {',', ' '};
		array <String ^> ^NewBackupOrderValues = BackupOrderTextBox->Text->Split(sep, StringSplitOptions::RemoveEmptyEntries);

		for (int i = 0; i < NewBackupOrderValues->Length; i++) {
			BOKey->SetValue(Convert::ToString(i+1), Convert::ToUInt32(NewBackupOrderValues[i], 10), RegistryValueKind::DWord);
		}

		//Ordinary String Values
		rzbkey->SetValue("Log.File", LogFilePathTextBox->Text);
		rzbkey->SetValue("Global.TransferPassword", TransferPasswordTextBox->Text);	
		rzbkey->SetValue("Locality.BlockStore", StoragePathTextBox->Text);		
		rzbkey->SetValue("MessageQueue.Host", HostTextBox->Text);	
		rzbkey->SetValue("MessageQueue.Username", UsernameTextBox->Text);	
		rzbkey->SetValue("MessageQueue.Password", PasswordTextBox->Text);

		populateData();
			
	}
}
