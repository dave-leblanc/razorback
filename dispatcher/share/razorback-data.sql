-- This SQL source script populates the database with the appropriate values
-- to support the default nuggets, data types and metadata for the base
-- Razorback system install

LOCK TABLES Nugget_Type WRITE;

INSERT IGNORE INTO `Nugget_Type` (`Name`, `UUID`, `Description`) VALUES 
    ("CORRELATION", "2fd75fa5-778b-443e-b910-1e19044e81e1", "Correlation Nugget"),
    ("INTEL",       "356112d8-f4f1-41dc-b3f7-cace5674c2ec", "Intel Nugget"),
    ("DEFENSE",     "5e9c1296-ad6a-423f-9eca-9f817c72c444", "Defense Update Nugget"),
    ("OUTPUT",      "a3d0d1f9-c049-474e-bf01-2128ea00a751", "Output Nugget"),
    ("COLLECTION",  "c38b113a-27fd-417c-b9fa-f3aa0af5cb53", "Data Collector Nugget"),
    ("INSPECTION",  "d95aee72-9186-4236-bf23-8ff77dac630b", "Inspection Nugget"),
    ("DISPATCHER",  "1117de3c-6fe8-4291-84ae-36cdf2f91017", "Message Dispatcher Nugget"),
    ("MASTER",      "ca51afd1-41b8-4c6b-b221-9faef0d202a7", "Master Nugget");
UNLOCK TABLES;

LOCK TABLES App_Type WRITE, Nugget_Type WRITE;

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "ADWARE_EULA", "25ae7c32-2282-4345-9818-6582dea8a2d7", `UUID`, "Ruby script to find adware EULAs"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "ARCHIVE_INFLATE", "849942cd-ee07-4ffd-a8d9-f1dcf1f768c1", `UUID`, "Archive Inflator"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "EMU_SCRIPT", "2d96bc81-1171-447f-9b95-4fc155aaa528", `UUID`, "Perl script using libemu sctest"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "CLAMAV",          "be312018-1b8e-462e-90a8-983a036c8e8c", `UUID`, "ClamAV Scanner"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "FILE_INJECT",     "be312018-1b8e-462e-90a8-983a036c8e00", `UUID`, "File Injector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "FLASH_INSPECTOR", "20f75380-afb0-11df-866f-7b86c649596b", `UUID`, "Flash Inspector"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "FS_MONITOR",      "b1856e90-afb1-11df-a7b9-db6ce92e17a0", `UUID`, "File System Monitor Collector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "FS_WALKER",       "fbd73e9a-b521-11df-9821-001c2309dba9", `UUID`, "File System Walker Collector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "LIBEMU",          "40566082-afaf-11df-9ed5-e3ab45e014a4", `UUID`, "LibEMU Shellcode Detector"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "OFFICECAT",       "73392f48-afaf-11df-a519-8b58a542a7e9", `UUID`, "OfficeCat"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "PDF_DISSECTOR",   "8d412e36-afaf-11df-b123-fbc142fa749e", `UUID`, "Zynamics PDF Dissector"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`)
    SELECT
        "PDF_FOX",   "d721e5f0-a5b7-4cea-9eae-4111117d72c4", `UUID`, "Sourcefire PDF Fox"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "POSTFIX",         "fea51a18-afb1-11df-ba4b-9ffffd3fafdb", `UUID`, "PostFix Integrated Collector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SENDMAIL_MILTER", "1a1159ba-afb2-11df-8a74-d37238e261d7", `UUID`, "Sendmail Milter Integrated Collector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SMTP_PARSER",     "7ab0669b-3782-5888-b451-3479bfb6cdd4", `UUID`, "SMTP Parser"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SNORT_COLLECTOR", "a3b124f9-ee08-5fdf-8f3e-5e8ddde083d5", `UUID`, "Snort As A Collector (SAAC)"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SQUID_COLLECTOR", "9d98bf86-afb1-11df-bb5e-2f259b27f5ea", `UUID`, "Squid Integrated Collector"
    FROM Nugget_Type WHERE `Name`="COLLECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "STEG_DETECT",     "e3be7a1e-afb1-11df-8c80-8b75af315b73", `UUID`, "Stegdetect Detection Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "VIRUS_TOTAL",     "926b5bea-afb2-11df-8007-fb6cd1236c62", `UUID`, "Virus Total Integrated Inspector"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "YARA",            "2f631118-42cf-11e0-83c8-000c298fbda4", `UUID`, "Yara Integrated Detection Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "FILE_LOG",        "74c135c6-50b9-11e0-9ed2-e31d9c72c9f1", `UUID`, "File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "PDF_LOG_1",       "83fb62c6-2576-4a54-a3d1-e6a261e5ed94", `UUID`, "PDF 1 File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "PDF_LOG_2",       "6cae9c8d-5a40-4269-ab2a-2d6c490dd849", `UUID`, "PDF 2 File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "PDF_LOG_3",       "21a2090b-c2db-4ff6-92eb-d8768580cfa9", `UUID`, "PDF 3 File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SWF_LOG_1",       "f1476e4a-d88e-499f-acee-27cfbe1f44be", `UUID`, "SWF 1 File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SWF_LOG_2",       "32230837-7005-46be-8b5d-5e0148075e68", `UUID`, "SWF 2 File Logging Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";
    
INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "RUBY_SAMPLE",     "869838f6-b6e1-11e0-b208-b735188dcb19", `UUID`, "Sample detection nugget written in Ruby"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "SYSLOG",   "de39e60b-073f-4598-a275-2831013e55eb", `UUID`, "Syslog alert output nugget"
    FROM Nugget_Type WHERE `Name`="OUTPUT";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "AVG_NUGGET",     "cf40e4cf-9d07-4ff5-9bb0-392289dedcb5", `UUID`, "AVG Scanning Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "AVAST_NUGGET",     "d89b1795-d318-4ea1-b004-afbf3ac8f79d", `UUID`, "Avast Scanning Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`) 
    SELECT 
        "AVIRA_NUGGET",     "b4fd91d5-2993-41ed-b93e-49ab6f037412", `UUID`, "Avira Scanning Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`)
    SELECT
        "KASPERSKY_NUGGET",     "dcb1d81f-e44d-4461-b17c-6365c0f5ff24", `UUID`, "Kaspersky Scanning Nugget"
    FROM Nugget_Type WHERE `Name`="INSPECTION";

INSERT IGNORE INTO `App_Type` (`Name`, `UUID`, `Nugget_Type_UUID`, `Description`)
    SELECT
        "RAZORD",     "5125ad5b-6560-4fda-b997-c0d9ffcb06e1", `UUID`, "Razorback Collection Daemon"
    FROM Nugget_Type WHERE `Name`="COLLECTION";



UNLOCK TABLES;

LOCK TABLES Nugget WRITE, App_Type WRITE, Nugget_Type WRITE;

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'fdb7fcb8-e468-4d8e-bea4-d1422c406d6e',
        `UUID`, `Nugget_Type_UUID`, 
        'Sample adware analysis nugget',
        'Sourcefire', 'Sourcefire',
        'Sample Ruby scripting nugget'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='ADWARE_EULA';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '2d93507e-b497-478c-8d90-7ac407552433',
        `UUID`, `Nugget_Type_UUID`, 
        'Sample PDF Dissector nugget', 
        'Sourcefire', 'Sourcefire',
        'Sample PDF Analysis nugget'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='PDF_DISSECTOR';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`)
    SELECT
        '1ceb82dd-543b-4117-a9d8-45768e38f310',
        `UUID`, `Nugget_Type_UUID`,
        'Sample PDF Fox nugget',
        'Sourcefire', 'Sourcefire',
        'Sample PDF Fox Analysis nugget'
        FROM `App_Type`
            WHERE `App_Type`.`Name`='PDF_FOX';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'ae332b6d-7310-4725-a1f3-5da3dbd07222',
        `UUID`, `Nugget_Type_UUID`, 
        'Sample Ruby Nugget',
        'Sourcefire', 'Sourcefire',
        'Sample Ruby scripting nugget'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='RUBY_SAMPLE';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'cf52e719-91de-456c-be4c-9c758f4389ca',
        `UUID`, `Nugget_Type_UUID`, 
        'Sample shellcode processing Perl nugget',
        'Sourcefire', 'Sourcefire',
        'Sample shellcode processing nugget'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='EMU_SCRIPT';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'c1cb38b8-2847-4bff-955c-476450c07efe',
        `UUID`, `Nugget_Type_UUID`, 
        'PDF 1 Sample',
        'Sourcefire', 'Sourcefire',
        'Sample PDF Log 1 Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='PDF_LOG_1';


INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '03a46c05-a34a-4a6a-9fa1-0dd1bb1fe74f', 
        `UUID`, `Nugget_Type_UUID`, 
        'PDF 2 Sample',
        'Sourcefire', 'Sourcefire',
        'Sample PDF Log 2 Nugget UUID' 
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='PDF_LOG_2';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'eec952fc-1856-4411-b19b-3f264bc67722',
        `UUID`, `Nugget_Type_UUID`, 
        'PDF 3 Sample',
        'Sourcefire', 'Sourcefire',
        'Sample PDF Log 3 Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='PDF_LOG_3';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'bfad6070-e695-4382-b1b4-7e428aa478c7',
        `UUID`, `Nugget_Type_UUID`, 
        'SWF 1 Sample',
        'Sourcefire', 'Sourcefire',
        'Sample SWF Log 1 Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='SWF_LOG_1';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '2e229373-78ec-4044-b770-4ce5ca7aa1a9',
        `UUID`, `Nugget_Type_UUID`, 
        'SWF 2 Sample',
        'Sourcefire', 'Sourcefire',
        'Sample SWF Log 2 Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='SWF_LOG_2';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '4e7e65f9-e0e3-4329-9ec6-eb01c763440e',
        `UUID`, `Nugget_Type_UUID`, 
        'File Inject Sample',
        'Sourcefire', 'Sourcefire',
        'Sample File Inject Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='FILE_INJECT';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '4e7e65f9-e0e3-4329-9ec6-eb01c763440f',
        `UUID`, `Nugget_Type_UUID`, 
        'Fs Monitor Sample',
        'Sourcefire', 'Sourcefire',
        'Sample FS Monitor Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='FS_MONITOR';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'a91cc89f-4a5d-462a-9ea5-e94f756297c8',
        `UUID`, `Nugget_Type_UUID`, 
        'Fs Walk Sample',
        'Sourcefire', 'Sourcefire',
        'Sample FS Walk Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='FS_WALKER';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '4e7e65f9-e0e3-4329-9ec6-eb01c763440d',
        `UUID`, `Nugget_Type_UUID`, 
        'Snort Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Snort Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='SNORT_COLLECTOR';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '6867cc23-92a8-48a9-b9e4-47169dcdc6a1',
        `UUID`, `Nugget_Type_UUID`, 
        'File Log Sample',
        'Sourcefire', 'Sourcefire',
        'Sample File Log Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='FILE_LOG';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '6867cc23-92a8-48a9-b9e4-47169dcdc6a3',
        `UUID`, `Nugget_Type_UUID`, 
        'ClamAV Sample',
        'Sourcefire', 'Sourcefire',
        'Sample ClamAV Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='CLAMAV';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '849942cd-ee07-4ffd-a8d9-f1dcf1f768c1',
        `UUID`, `Nugget_Type_UUID`, 
        'Archive Inflate Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Archive Inflate Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='ARCHIVE_INFLATE';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '00a17643-5785-41b4-b661-71521a6092a3',
        `UUID`, `Nugget_Type_UUID`, 
        'LibEMU Sample',
        'Sourcefire', 'Sourcefire',
        'Sample LibEMU Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='LIBEMU';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '5b0d7e61-7b18-487f-94b9-0fad4f22bd50',
        `UUID`, `Nugget_Type_UUID`, 
        'Virus Total Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Virus Total Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='VIRUS_TOTAL';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '526d9b51-95b9-4447-acaa-972a603e6810',
        `UUID`, `Nugget_Type_UUID`, 
        'OfficeCat Sample',
        'Sourcefire', 'Sourcefire',
        'Sample OfficeCat Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='OFFICECAT';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '428d34c2-95c4-438c-ba10-f8684b68e109',
        `UUID`, `Nugget_Type_UUID`, 
        'Yara Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Yara Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='YARA';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '9e980b1c-cea5-4270-b93b-cd12142a7cdc',
        `UUID`, `Nugget_Type_UUID`, 
        'SWF Scanner Sample',
        'Sourcefire', 'Sourcefire',
        'Sample SWF Scanner Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='FLASH_INSPECTOR';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '8d9ef049-4af7-4362-beb7-7d63e12823f6',
        NULL, `UUID`,
        'Master Nugget Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Master Nugget UUID'
        FROM `Nugget_Type` 
            WHERE `Nugget_Type`.`Name`='MASTER';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '61f18be6-ad5a-42dd-807c-fe9681ba738d',
        NULL, `UUID`,
        'Dispatcher Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Dispatcher UUID'
        FROM `Nugget_Type` 
            WHERE `Nugget_Type`.`Name`='DISPATCHER';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'e2bd940f-6c6d-4b26-b1cb-fc4f59422b23',
        NULL, `UUID`,
        'Dispatcher Sample 2',
        'Sourcefire', 'Sourcefire',
        'Sample Dispatcher 2 UUID'
        FROM `Nugget_Type` 
            WHERE `Nugget_Type`.`Name`='DISPATCHER';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '13494958-cd46-4d48-94cc-952579ec86b3',
        `UUID`, `Nugget_Type_UUID`, 
        'Syslog Output Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Syslog Output Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='SYSLOG';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '237a8e5c-9ffd-4a3d-84d9-e47acee7659a',
        `UUID`, `Nugget_Type_UUID`, 
        'AVG Scanner Sample',
        'Sourcefire', 'Sourcefire',
        'Sample AVG Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='AVG_NUGGET';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`)
    SELECT
        '60562e84-a07e-4ef1-bbb8-c1142439485b',
        `UUID`, `Nugget_Type_UUID`,
        'Kasperky Scanner Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Kaspersky Nugget UUID'
        FROM `App_Type`
            WHERE `App_Type`.`Name`='KASPERSKY_NUGGET';


INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '8b2d0259-4e05-47ca-9cc8-c96fb186b335',
        `UUID`, `Nugget_Type_UUID`, 
        'Avast Scanner Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Avast Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='AVAST_NUGGET';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        'efbf1e24-78d1-4917-a0c2-3fa36fc09288',
        `UUID`, `Nugget_Type_UUID`, 
        'Avira Scanner Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Avira Nugget UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='AVIRA_NUGGET';

INSERT IGNORE INTO Nugget (`UUID`, `App_Type_UUID`, `Nugget_Type_UUID`, `Name`, `Location`, `Contact`, `Notes`) 
    SELECT  
        '99a90c87-bdb5-4ae6-9d8a-5af0b17cdd6a',
        `UUID`, `Nugget_Type_UUID`, 
        'Razorback Collection Daemon Sample',
        'Sourcefire', 'Sourcefire',
        'Sample Razorback Collection UUID'
        FROM `App_Type` 
            WHERE `App_Type`.`Name`='RAZORD';

UNLOCK TABLES;

LOCK TABLES UUID_Data_Type WRITE;
INSERT IGNORE INTO UUID_Data_Type VALUES ('00000000-0000-0000-0000-000000000000','ANY_DATA','Any Data');
INSERT IGNORE INTO UUID_Data_Type VALUES ('005d5464-7a44-4907-af57-4db08a61e13c','PDF_FILE','PDF Document');
INSERT IGNORE INTO UUID_Data_Type VALUES ('16d72948-3d2b-52ed-9ae4-43ef19ce3e69','OLE_FILE','OLE Document');
INSERT IGNORE INTO UUID_Data_Type VALUES ('2858f242-1d4e-4f80-9744-b3fed26b9d21','ISO9660_FILE','Iso9660 CD Image');
INSERT IGNORE INTO UUID_Data_Type VALUES ('2b1b387e-4c8a-490f-9d9d-bea6dea90593','GIF_FILE','GIF File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('2b797454-d367-4841-8c9c-a713d012b546','JAVASCRIPT','JavaScript');
INSERT IGNORE INTO UUID_Data_Type VALUES ('2d76a190-afb2-11df-91ba-1b85bf3d7e00','TAR_FILE','TAR Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('36966d78-afb2-11df-bfc6-1b9c2f1ce37a','RAR_FILE','RAR Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('3ee61f14-afb2-11df-b122-339e7fae4010','PAR_FILE','PAR Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('4cc82ee2-afb2-11df-bf91-4f6797c414f4','PAR2_FILE','PAR2 Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('4e72c8ec-ff88-4371-a0f0-dfe2b4c733dc','SHELL_CODE','Suspected Shellcode');
INSERT IGNORE INTO UUID_Data_Type VALUES ('52b2d8a8-8ccc-45af-91d3-74183f5f5f63','CPIO_FILE','CPIO Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('52f738c2-db1c-42ca-b5ef-50a4ba3f7527','ZIP_FILE','ZIP Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('532cf708-e293-4519-8796-d583a12a8fe1','JPEG_FILE','JPEG 2000 File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('575daa2a-2ada-42bb-818e-1ff6c36fef41','XZ_FILE','Xz Compressed File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('7966e375-e657-49ca-86b9-da870dd10809','PNG_FILE','PNG File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('7ab45fff-7c73-412c-8b86-c07619c8fc7d','FLASH_FILE','Adobe Flash');
INSERT IGNORE INTO UUID_Data_Type VALUES ('7e2a3a7c-69f5-11e0-8186-737fbe930662','WIN_HELP_FILE','Windows Help File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('a304dfd7-340e-4b96-b9af-ce0cb775f148','LZMA_FILE','Lzma Compressed File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('a6f78a4c-46c5-486c-a00a-780a8cf25e6d','BZ2_FILE','Bzip2 Compressed File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('b4b02352-149a-4d3b-811a-1c707ba7dd70','HTML_FILE','HTML File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('ba8b6784-1412-494d-91bb-d2868ecbb196','AR_FILE','Ar Archive File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('ba9beb5f-0653-4b04-9552-3bfb634ca7fc','PE_FILE','PE Executable');
INSERT IGNORE INTO UUID_Data_Type VALUES ('d00b0f8b-2e7a-4808-81c2-a19e86b4b4fd','GZIP_FILE','Gzip Compressed File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('d147f215-128e-4746-a1e2-b6c978bb1869','SMTP_CAPTURE','SMTP Mail Capture');
INSERT IGNORE INTO UUID_Data_Type VALUES ('fff8d04d-90e7-4eaf-be33-31b2c7e4255d','COMPRESSION_FILE','Compression Compressed File');
INSERT IGNORE INTO UUID_Data_Type VALUES ('15961cf0-78b4-4024-ae48-29ad2c86fb4b','ELF_FILE','ELF Executable File');
UNLOCK TABLES;

LOCK TABLES UUID_Metadata_Name WRITE;
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('10bdfb54-2503-4157-a73e-564927566b00','DEST','Destination');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('70991a40-09d6-442d-824a-1c6e642ec552','SOURCE','Source');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('ad515154-f6d4-4f68-9ec3-d4cf89b4c9d7','PATH','Path');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('b804c4c4-2801-49a3-a1eb-0a84318686ef','FILENAME','File Name');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('c79e3426-b00e-4a2c-ada6-b3f67354e169','HOSTNAME','Hostname');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('d52c4eca-71a9-4152-8cb6-692a71ef0b83','MALWARENAME','Name of detected malware');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('ad515154-f6d4-4f68-9ec3-d4cf89b4c9d8','REPORT','Report');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('ad515154-f6d4-4f68-9ec3-d4cf89b4c9d9','CVE','CVE');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('870e6920-c24c-4c66-8768-320893866d4c','BID','Bugtraq ID');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('0999d503-e293-46b1-8211-b262ff0f832c','OSVDB','OSVDB ID');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('9077005b-895d-47fa-abb7-19dd9d4aa6b1','URI','URI');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('04d64a08-479e-4d64-939d-572897025a6b','HTTP_REQUEST','HTTP Request');
INSERT IGNORE INTO UUID_Metadata_Name VALUES ('0c865866-ae6b-4740-a80c-b0b0635c0930','HTTP_RESPONSE','HTTP Response');
UNLOCK TABLES;


LOCK TABLES UUID_Metadata_Type WRITE;
INSERT IGNORE INTO UUID_Metadata_Type VALUES ('101e30d7-e6ee-4171-999a-c4d99325a93e','IPv4_ADDR','IPv4 Address', NULL);
INSERT IGNORE INTO UUID_Metadata_Type VALUES ('3590b149-859a-4625-9d01-749346ddf103','IPv6_ADDR','IPv6 Address', NULL);
INSERT IGNORE INTO UUID_Metadata_Type VALUES ('5375a0ec-b7ee-4985-b23d-642556b75543','IP_PROTO','IP Protocol', NULL);
INSERT IGNORE INTO UUID_Metadata_Type VALUES ('b0abffef-01d6-487b-bff5-a9e62b17c5b6','PORT','Port', NULL);
INSERT IGNORE INTO UUID_Metadata_Type VALUES ('c24ceea9-3906-47f3-97c4-78cb881b8de8','STRING','String', NULL);


UNLOCK TABLES;

LOCK TABLES Locality WRITE;
INSERT IGNORE INTO Locality VALUES
    (1, "Unconnected", "", "", "");
update Locality set Locality_ID=0 where Locality_ID=1;
INSERT IGNORE INTO Locality VALUES
    (1, "Default", "", "", "");
UNLOCK TABLES;
