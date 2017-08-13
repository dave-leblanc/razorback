-- MySQL dump 10.13  Distrib 5.1.49, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: razorback
-- ------------------------------------------------------
-- Server version	5.1.49-1ubuntu8.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `Block`
--

DROP TABLE IF EXISTS `Block`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Block` (
  `Block_ID` bigint(20) NOT NULL AUTO_INCREMENT,
  `Data_Type_UUID` char(36) NOT NULL,
  `Hash` blob NOT NULL,
  `Size` int(10) unsigned NOT NULL,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `SF_Flags` int(10) unsigned not NULL default 0,
  `ENT_Flags` int(10) unsigned not NULL default 0,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`Block_ID`),
  UNIQUE KEY `Block_Size_Hash_Key` (`Hash`(512),`Size`),
  KEY `Block_Hash_Index` (`Hash`(512)),
  KEY `Block_Size_Index` (`Size`),
  KEY `Block_FKIndex1` (`Metabook_ID`),
  KEY `Block_FKIndex2` (`Data_Type_UUID`),
  CONSTRAINT `Block_ibfk_1` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Block_ibfk_2` FOREIGN KEY (`Data_Type_UUID`) REFERENCES `UUID_Data_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Event`
--

DROP TABLE IF EXISTS `Event`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Event` (
  `Event_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Nugget_UUID` char(36) NOT NULL,
  `Block_ID` bigint(20) DEFAULT NULL,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Time_Secs` bigint(20) NOT NULL DEFAULT '0',
  `Time_NanoSecs` bigint(20) NOT NULL DEFAULT '0',
  PRIMARY KEY (`Event_ID`),
  KEY `Event_FKIndex1` (`Block_ID`),
  KEY `Event_FKIndex2` (`Nugget_UUID`),
  KEY `Event_FKIndex3` (`Metabook_ID`),
  KEY `Insp` (`Time_Secs`, `Time_NanoSecs`, `Nugget_UUID`),
  CONSTRAINT `Event_ibfk_1` FOREIGN KEY (`Block_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Event_ibfk_2` FOREIGN KEY (`Nugget_UUID`) REFERENCES `Nugget` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Event_ibfk_3` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Block_Inspection`
--

DROP TABLE IF EXISTS `Block_Inspection`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Block_Inspection` (
  `Block_ID` bigint(20) NOT NULL,
  `Event_ID` bigint(20) unsigned NOT NULL,
  `Inspector_Type_UUID` char(36) NOT NULL,
  `Inspector_UUID` char(36),
  `Status` tinyint NOT NULL DEFAULT 0,
  key `Block_Inspection_ID_Insp_Status` (`Block_ID`, `Inspector_UUID`, `Status`),
  KEY `Block_Inspection_ibfk_1` (`Block_ID`),
  KEY `Block_Inspection_ibfk_2` (`Event_ID`),
  KEY `Block_Inspection_ibfk_3` (`Inspector_Type_UUID`),
  KEY `Block_Inspection_ibfk_4` (`Inspector_UUID`),
  CONSTRAINT `Block_Inspection_ibfk_1` FOREIGN KEY (`Block_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Block_Inspection_ibfk_2` FOREIGN KEY (`Event_ID`) REFERENCES `Event` (`Event_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Block_Inspection_ibfk_3` FOREIGN KEY (`Inspector_Type_UUID`) REFERENCES `App_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Block_Inspection_ibfk_4` FOREIGN KEY (`Inspector_UUID`) REFERENCES `Nugget` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;


--
-- Table structure for table `Event_Address_Info`
--

DROP TABLE IF EXISTS `Connection_Info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Connection_Info` (
  `Metabook_ID` bigint(20) NOT NULL,
  `Source_Address_ID` bigint(20) unsigned DEFAULT NULL,
  `Destination_Address_ID` bigint(20) unsigned DEFAULT NULL,
  `Source_Port` int(10) unsigned DEFAULT NULL,
  `Destination_Port` int(10) unsigned DEFAULT NULL,
  `Protocol` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`Metabook_ID`),
  KEY `Address_Info_FKIndex1` (`Source_Address_ID`),
  KEY `Address_Info_FKIndex2` (`Destination_Address_ID`),
  KEY `Address_Info_FKIndex3` (`Metabook_ID`),
  CONSTRAINT `Event_Address_Info_ibfk_1` FOREIGN KEY (`Source_Address_ID`) REFERENCES `IP_Address` (`IP_Address_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Event_Address_Info_ibfk_2` FOREIGN KEY (`Destination_Address_ID`) REFERENCES `IP_Address` (`IP_Address_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Metabook_Address_Info_ibfk_3` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Host`
--

DROP TABLE IF EXISTS `Host`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Host` (
  `Host_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Name` char(255) DEFAULT NULL,
  `SF_Flags` int(10) unsigned not NULL default 0,
  `ENT_Flags` int(10) unsigned not NULL default 0,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`Host_ID`),
  UNIQUE KEY `Host_Name` (`Name`),
  KEY `Host_FKIndex1` (`Metabook_ID`),
  CONSTRAINT `Host_ibfk_1` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `IP_Address`
--

DROP TABLE IF EXISTS `IP_Address`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `IP_Address` (
  `IP_Address_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Address_Type` enum('IPv6','IPv4') NOT NULL,
  `Address` tinyblob NOT NULL,
  `SF_Flags` int(10) unsigned not NULL default 0,
  `ENT_Flags` int(10) unsigned not NULL default 0,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`IP_Address_ID`),
  UNIQUE KEY `IP_Address_Address_Type` (`Address_Type`,`Address`(16)),
  KEY `IP_Address_Type` (`Address_Type`),
  KEY `IP_Address_Address` (`Address`(16)),
  KEY `IP_Address_FKIndex1` (`Metabook_ID`),
  CONSTRAINT `IP_Address_ibfk_1` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LK_Block_Block`
--

DROP TABLE IF EXISTS `LK_Block_Block`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LK_Block_Block` (
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Parent_ID` bigint(20) NOT NULL,
  `Child_ID` bigint(20) NOT NULL,
  `SF_Flags` int(10) unsigned not NULL default 0,
  `ENT_Flags` int(10) unsigned not NULL default 0,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`Parent_ID`,`Child_ID`),
  KEY `LK_Block_Block_Primary` (`Parent_ID`,`Child_ID`),
  KEY `LK_Block_Block_FKIndex1` (`Parent_ID`),
  KEY `LK_Block_Block_FKIndex2` (`Child_ID`),
  KEY `LK_Block_Block_FKIndex3` (`Metabook_ID`),
  CONSTRAINT `LK_Block_Block_ibfk_1` FOREIGN KEY (`Parent_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Block_Block_ibfk_2` FOREIGN KEY (`Child_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Block_Block_ibfk_3` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LK_Event_Event`
--

DROP TABLE IF EXISTS `LK_Event_Event`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LK_Event_Event` (
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Parent_ID` bigint(20) unsigned NOT NULL,
  `Child_ID` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`Parent_ID`,`Child_ID`),
  KEY `LK_Event_Event_Primary` (`Parent_ID`,`Child_ID`),
  KEY `LK_Event_Event_FKIndex1` (`Parent_ID`),
  KEY `LK_Event_Event_FKIndex2` (`Child_ID`),
  KEY `LK_Event_Event_FKIndex3` (`Metabook_ID`),
  CONSTRAINT `LK_Event_Event_ibfk_1` FOREIGN KEY (`Parent_ID`) REFERENCES `Event` (`Event_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Event_Event_ibfk_2` FOREIGN KEY (`Child_ID`) REFERENCES `Event` (`Event_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Event_Event_ibfk_3` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LK_Metabook_Host`
--

DROP TABLE IF EXISTS `LK_Metabook_Host`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LK_Event_Host` (
  `Metabook_ID` bigint(20) NOT NULL AUTO_INCREMENT,
  `Host_ID` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`Metabook_ID`),
  UNIQUE KEY `LK_Metabook_Host_ID` (`Host_ID`),
  KEY `LK_Metabook_Host_FKIndex1` (`Metabook_ID`),
  KEY `LK_Metabook_Host_FKIndex2` (`Host_ID`),
  CONSTRAINT `LK_Metabook_Host_ibfk_1` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Metabook_Host_ibfk_2` FOREIGN KEY (`Host_ID`) REFERENCES `Host` (`Host_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LK_Event_URI`
--

DROP TABLE IF EXISTS `LK_Metabook_URI`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LK_Metabook_URI` (
  `Metabook_ID` bigint(20) NOT NULL AUTO_INCREMENT,
  `URI_ID` bigint(20) unsigned NOT NULL,
  PRIMARY KEY (`Metabook_ID`),
  KEY `LK_Metabook_URI_FKIndex1` (`Metabook_ID`),
  KEY `LK_Metabook_URI_FKIndex2` (`URI_ID`),
  CONSTRAINT `LK_Metabook_URI_ibfk_1` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Metabook_URI_ibfk_2` FOREIGN KEY (`URI_ID`) REFERENCES `URI` (`URI_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Metabook`
--

DROP TABLE IF EXISTS `Metabook`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Metabook` (
  `Metabook_ID` bigint(20) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`Metabook_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Metadata`
--

DROP TABLE IF EXISTS `Metadata`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Metadata` (
  `Metadata_ID` bigint(20) NOT NULL AUTO_INCREMENT,
  `Metadata_Type_UUID` char(36) NOT NULL,
  `Metadata_Name_UUID` char(36) NOT NULL,
  `Metabook_ID` bigint(20) NOT NULL,
  `Metadata` blob,
  PRIMARY KEY (`Metadata_ID`),
  KEY `Metadata_Metabook` (`Metabook_ID`),
  KEY `Metadata_FKIndex2` (`Metadata_Name_UUID`),
  KEY `Metadata_FKIndex3` (`Metadata_Type_UUID`),
  CONSTRAINT `Metadata_ibfk_1` FOREIGN KEY (`Metadata_Type_UUID`) REFERENCES `UUID_Metadata_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Metadata_ibfk_2` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Metadata_ibfk_3` FOREIGN KEY (`Metadata_Name_UUID`) REFERENCES `UUID_Metadata_Name` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Nugget`
--

DROP TABLE IF EXISTS `Nugget_Type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Nugget_Type` (
  `UUID` char(36) NOT NULL,
  `Name` char(100) DEFAULT NULL,
  `Description` text,
  PRIMARY KEY (`UUID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TABLE IF EXISTS `App_Type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `App_Type` (
  `UUID` char(36) NOT NULL,
  `Nugget_Type_UUID` char(36) NOT NULL,
  `Name` char(100) DEFAULT NULL,
  `Description` text,
  PRIMARY KEY (`UUID`), 
  KEY `App_Type_FKIndex1` (`Nugget_Type_UUID`),
  CONSTRAINT `App_Type_ibfk_1` FOREIGN KEY (`Nugget_Type_UUID`) REFERENCES `Nugget_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Nugget`
--
DROP TABLE IF EXISTS `Nugget`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Nugget` (
  `UUID` char(36) NOT NULL,
  `App_Type_UUID` char(36),
  `Nugget_Type_UUID` char(36) NOT NULL,
  `Name` char(100) DEFAULT NULL,
  `Location` text,
  `Contact` text,
  `Notes` text,
  `State` int(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`UUID`),
  KEY `Nugget_FKIndex1` (`App_Type_UUID`),
  KEY `Nugget_FKIndex2` (`Nugget_Type_UUID`),
  CONSTRAINT `Nugget_ibfk_1` FOREIGN KEY (`App_Type_UUID`) REFERENCES `App_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Nugget_ibfk_2` FOREIGN KEY (`Nugget_Type_UUID`) REFERENCES `Nugget_Type` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Protocol`
--

DROP TABLE IF EXISTS `Protocol`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Protocol` (
  `Protocol_ID` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `Name` char(100) NOT NULL,
  PRIMARY KEY (`Protocol_ID`),
  UNIQUE KEY `Protocol_Name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `URI`
--

DROP TABLE IF EXISTS `URI`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `URI` (
  `URI_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Path` text,
  `File_Name` text,
  `Protocol_ID` int(10) unsigned NOT NULL,
  `SF_Flags` int(10) unsigned not NULL default 0,
  `ENT_Flags` int(10) unsigned not NULL default 0,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`URI_ID`),
  UNIQUE KEY `URI_Path_Name_Proto` (`Path`(200),`File_Name`(200),`Protocol_ID`),
  KEY `URI_Proto` (`Protocol_ID`),
  KEY `URI_File` (`File_Name`(200)),
  KEY `URI_Path` (`Path`(200)),
  KEY `URI_FKIndex1` (`Protocol_ID`),
  KEY `URI_FKIndex2` (`Metabook_ID`),
  CONSTRAINT `URI_ibfk_1` FOREIGN KEY (`Protocol_ID`) REFERENCES `Protocol` (`Protocol_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `URI_ibfk_2` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `UUID_Data_Type`
--

DROP TABLE IF EXISTS `UUID_Data_Type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `UUID_Data_Type` (
  `UUID` char(36) NOT NULL,
  `Name` char(32) DEFAULT NULL,
  `Description` text,
  PRIMARY KEY (`UUID`),
  UNIQUE KEY `UUID_Data_Type_Name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `UUID_Metadata_Name`
--

DROP TABLE IF EXISTS `UUID_Metadata_Name`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `UUID_Metadata_Name` (
  `UUID` char(36) NOT NULL,
  `Name` char(32) NOT NULL,
  `Description` text,
  PRIMARY KEY (`UUID`),
  UNIQUE KEY `UUID_MetadataName_Name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `UUID_Metadata_Type`
--

DROP TABLE IF EXISTS `UUID_Metadata_Type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `UUID_Metadata_Type` (
  `UUID` char(36) NOT NULL,
  `Name` char(32) DEFAULT NULL,
  `Description` text,
  `Data_Size` int(11) DEFAULT NULL,
  PRIMARY KEY (`UUID`),
  KEY `UUIDs_Name` (`Name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Alert`
--

DROP TABLE IF EXISTS `Alert`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Alert` (
  `Alert_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Nugget_UUID` char(36) NOT NULL,
  `Event_ID` bigint(20) unsigned DEFAULT NULL,
  `Block_ID` bigint(20) DEFAULT NULL,
  `Metabook_ID` bigint(20) DEFAULT NULL,
  `Time_Secs` bigint(20) NOT NULL DEFAULT '0',
  `Time_NanoSecs` bigint(20) NOT NULL DEFAULT '0',
  `GID` int(10) unsigned DEFAULT NULL,
  `SID` int(10) unsigned DEFAULT NULL,
  `Priority` tinyint DEFAULT 0,
  `Message` text NOT NULL,
  PRIMARY KEY (`Alert_ID`),
  KEY `Alert_FKIndex1` (`Block_ID`),
  KEY `Alert_FKIndex2` (`Event_ID`),
  KEY `Alert_FKIndex3` (`Nugget_UUID`),
  KEY `Alert_FKIndex4` (`Metabook_ID`),
  CONSTRAINT `Alert_ibfk_1` FOREIGN KEY (`Block_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Alert_ibfk_2` FOREIGN KEY (`Event_ID`) REFERENCES `Event` (`Event_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Alert_ibfk_3` FOREIGN KEY (`Nugget_UUID`) REFERENCES `Nugget` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Alert_ibfk_4` FOREIGN KEY (`Metabook_ID`) REFERENCES `Metabook` (`Metabook_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Log_Message`
--

DROP TABLE IF EXISTS `Log_Message`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Log_Message` (
  `Message_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Nugget_UUID` char(36) NOT NULL,
  `Event_ID` bigint(20) unsigned DEFAULT NULL,
  `Timestamp` timestamp NOT NULL,
  `Message` text NOT NULL,
  `Priority` tinyint NOT NULL DEFAULT 0,
  PRIMARY KEY (`Message_ID`),
  KEY `Alert_FKIndex1` (`Event_ID`),
  KEY `Alert_FKIndex2` (`Nugget_UUID`),
  CONSTRAINT `Log_Message_ibfk_1` FOREIGN KEY (`Event_ID`) REFERENCES `Event` (`Event_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `Log_Message_ibfk_2` FOREIGN KEY (`Nugget_UUID`) REFERENCES `Nugget` (`UUID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Nugget`
--
DROP TABLE IF EXISTS `Locality`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Locality` (
  `Locality_ID` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  `Name` char(100) DEFAULT NULL,
  `Location` text,
  `Contact` text,
  `Notes` text,
  PRIMARY KEY (`Locality_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `LK_Block_Locality`
--

DROP TABLE IF EXISTS `LK_Block_Locality`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `LK_Block_Locality` (
  `Block_ID` bigint(20) NOT NULL,
  `Locality_ID` bigint(20) unsigned NOT NULL,
  `Timestamp` timestamp NOT NULL,
  PRIMARY KEY (`Block_ID`,`Locality_ID`),
  KEY `LK_Block_Locality_Primary` (`Block_ID`,`Locality_ID`),
  KEY `LK_Block_Locality_FKIndex1` (`Block_ID`),
  KEY `LK_Block_Locality_FKIndex2` (`Locality_ID`),
  CONSTRAINT `LK_Block_Locality_ibfk_1` FOREIGN KEY (`Block_ID`) REFERENCES `Block` (`Block_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `LK_Block_Locality_ibfk_2` FOREIGN KEY (`Locality_ID`) REFERENCES `Locality` (`Locality_ID`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

DROP TRIGGER IF EXISTS `upd_flags_to_link`;
delimiter //
CREATE TRIGGER upd_flags_to_link AFTER UPDATE ON Block
FOR EACH ROW
BEGIN
    IF (NEW.SF_Flags & 0x00000003) != (OLD.SF_Flags & 0x00000003)
    THEN
        UPDATE LK_Block_Block SET SF_Flags=((SF_Flags & ~0x00000003) | NEW.SF_Flags) WHERE Child_ID=NEW.Block_ID;
    END IF;
END;// 
delimiter ;


/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2011-06-30 10:04:31
