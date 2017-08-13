#pragma once

namespace TestUI {

	using namespace System;
	using namespace Microsoft::Win32;

public ref class RZBConfiguration : public System::Windows::Forms::TreeNode
{
public:
	RZBConfiguration(String ^name, RegistryKey ^key);
	void populateData();
	void initGlobalGroup();
	void initCacheGroup();
	void initLocalGroup();
	void initMQGroup();
	void initLogGroup();
	void populateGlobalGroup();
	void populateCacheGroup();
	void populateLocalGroup();
	void populateMQGroup();
	void populateLogGroup();
	void populateDefaults();
	void storeAll();

	System::Windows::Forms::TreeNode^ GlobalNode;
	System::Windows::Forms::TreeNode^ CacheNode;
	System::Windows::Forms::TreeNode^ LocalityNode;
	System::Windows::Forms::TreeNode^ MQNode;
	System::Windows::Forms::TreeNode^ LoggingNode;
	System::Windows::Forms::GroupBox^  GlobalGroupBox;
	System::Windows::Forms::GroupBox^  CacheGroupBox;
	System::Windows::Forms::GroupBox^  MessageQueueGroupBox;
	System::Windows::Forms::GroupBox^  LoggingGroupBox;
	System::Windows::Forms::GroupBox^  LocalityGroupBox;

private:

	RegistryKey ^rzbkey, ^basekey;
	array <String ^> ^BackupOrderValues;
	array <String ^> ^subvalues;
	String ^keyname;
	String ^TransferPasswordValue;
	String ^MaxThreadsValue;
	String ^HashTypeValue;
	String ^MaxBlockSizeValue;
	String ^HelloTimeValue;
	String ^DeadTimeValue;
	String ^MessageFormatValue;
	String ^LocalityIDValue;
	String ^BlockStoreValue;
	String ^BackupOrderValue;
	String ^BadLimitValue;
	String ^GoodLimitValue;
	String ^HostValue;
	String ^PortValue;
	String ^UsernameValue;
	String ^PasswordValue;
	String ^SSLValue;
	String ^DestinationValue;
	String ^LogLevelValue;
	String ^SyslogFacilityValue;
	String ^FilePathValue;

	
	System::Windows::Forms::Label^  MaxBlockLabel;
	System::Windows::Forms::TextBox^  MaxBlockTextBox;
	System::Windows::Forms::Label^  MaxThreadsLabel;
	System::Windows::Forms::TextBox^  MaxThreadsTextBox;
	System::Windows::Forms::Label^  TransferPasswordLabel;
	System::Windows::Forms::TextBox^  TransferPasswordTextBox;
	System::Windows::Forms::Label^  DeadLabel;
	System::Windows::Forms::Label^  HelloLabel;
	System::Windows::Forms::TextBox^  DeadTextBox;
	System::Windows::Forms::TextBox^  HelloTextBox;
	System::Windows::Forms::RadioButton^  SHA512Radio;
	System::Windows::Forms::RadioButton^  SHA256Radio;
	System::Windows::Forms::RadioButton^  SHA224Radio;
	System::Windows::Forms::Label^  HashTypeLabel;
	System::Windows::Forms::RadioButton^  SHA1Radio;
	System::Windows::Forms::RadioButton^  MD5Radio;
    System::Windows::Forms::Label^  MsgFormatLabel;
	System::Windows::Forms::RadioButton^  JSONRadio;
	System::Windows::Forms::RadioButton^  BinaryRadio; 
	System::Windows::Forms::TextBox^  LocalityIDTextBox;
	System::Windows::Forms::Label^  BackupOrderLabel;
	System::Windows::Forms::Label^  StoragePathLabel;
	System::Windows::Forms::TextBox^  StoragePathTextBox;
	System::Windows::Forms::TextBox^  BackupOrderTextBox;
	System::Windows::Forms::Label^  LocalityIDLabel;
	System::Windows::Forms::Label^  MaxGoodLabel;
	System::Windows::Forms::Label^  MaxBadLabel;
	System::Windows::Forms::TextBox^  MaxGoodTextBox;
	System::Windows::Forms::TextBox^  MaxBadTextBox;
	System::Windows::Forms::RadioButton^  SSLDisabledRadio;
	System::Windows::Forms::RadioButton^  SSLEnabledRadio;
	System::Windows::Forms::TextBox^  UsernameTextBox;
	System::Windows::Forms::TextBox^  PasswordTextBox;
	System::Windows::Forms::TextBox^  PortTextBox;
	System::Windows::Forms::TextBox^  HostTextBox;
	System::Windows::Forms::Label^  SSLLabel;
	System::Windows::Forms::Label^  PasswordLabel;
	System::Windows::Forms::Label^  UsernameLabel;
	System::Windows::Forms::Label^  PortLabel;
	System::Windows::Forms::Label^  HostLabel;
	System::Windows::Forms::Label^  LogFilePathLabel;
	System::Windows::Forms::Label^  SyslogFacilityLabel;
	System::Windows::Forms::Label^  LogLevelLabel;
	System::Windows::Forms::Label^  DestinationLabel;
	System::Windows::Forms::TextBox^  LogFilePathTextBox;
	System::Windows::Forms::ComboBox^  LogLevelComboBox;
	System::Windows::Forms::ComboBox^  SyslogFacilityComboBox;
	System::Windows::Forms::ComboBox^  DestinationComboBox;
	System::Windows::Forms::Panel^  HashTypePanel;
	System::Windows::Forms::Panel^  MsgFormatPanel;
	System::Windows::Forms::Panel^  SSLPanel;
};
}
