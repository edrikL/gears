#!/usr/bin/python2.4
#
# Copyright 2007 Google Inc. All Rights Reserved.

""" VS Project template for use with Templatizer class.

This file contains the templatized strings used for generating a VS project.
It is meant to be loaded via a call to Python's
execFile('filename.py', {}, dictToPlaceValuesFromThisFileIn)
executed by the Templatizer class.

As described in the documentation for the Templatizer class, each string 
contains {{identifier}} tags to be used for template substitution.

What follows is a definition of each template name defined in this file. For
each template we specify all parameters that can be replaced:

TARGET_SECTION: section defining a compilation target
  TARGET_NAME: human readable name for the target
  INCLUDE_DIRECTORIES: ; separated list of include directories
  PREPROCESSOR_DEFINES: ; separated list of preprocessor defines for that 
    target

MAIN_VS_PROJECT_SKELETON: main XML wrapper for project skeleton.
  TARGET_SECTIONS: place multiple target sections here.
  FILES_SECTION: place to put references to the actual files.

FILE_GROUP: definition of file group
  FILE_GROUP_NAME: name of this file group
  FILE_LIST: place list of files or more file groups in here.

FILE_DEFINITION: marker for an individual file
  REL_PATH_TO_FILE: path to file relative to the project. 
  FILE_EXCLUSIONS: zero or more BUILD_EXCLUDE_GROUPs

BUILD_EXCLUDE_GROUP: exclude current file from building in the specified
  environment.
   
EXCLUDE_TARGET_NAME: target to exclude this file in."""

__author__ = 'playmobil@google.com (Jeremy Moskovich)'

FILE_GROUP = r"""<Filter Name="{{FILE_GROUP_NAME}}">
	{{FILE_LIST}}
	</Filter>"""
	
# TODO(playmobil): this goes inside file tag to exclude files from a build 
# group.

BUILD_EXCLUDE_GROUP=r"""<FileConfiguration
  						Name="{{EXCLUDE_TARGET_NAME}}"
  						ExcludedFromBuild="true"
  						>
  						<Tool
  							Name="VCCLCompilerTool"
  						/>
  					</FileConfiguration>"""
	
FILE_DEFINITION = r"""<File
			RelativePath="{{REL_PATH_TO_FILE}}"
			>
			{{FILE_EXCLUSIONS}}
		</File>"""

TARGET_SECTION = r"""<Configuration
	Name="{{TARGET_NAME}}"
	OutputDirectory="$(SolutionDir)$(ConfigurationName)"
	IntermediateDirectory="$(ConfigurationName)"
	ConfigurationType="1"
	InheritedPropertySheets=
	"$(VCInstallDir)VCProjectDefaults\UpgradeFromVC71.vsprops"
	>
	<Tool
		Name="VCPreBuildEventTool"
	/>
	<Tool
		Name="VCCustomBuildTool"
	/>
	<Tool
		Name="VCXMLDataGeneratorTool"
	/>
	<Tool
		Name="VCWebServiceProxyGeneratorTool"
	/>
	<Tool
		Name="VCMIDLTool"
	/>
	<Tool
		Name="VCCLCompilerTool"
		AdditionalIncludeDirectories="{{INCLUDE_DIRECTORIES}}"
		PreprocessorDefinitions="{{PREPROCESSOR_DEFINES}}"
	/>
	<Tool
		Name="VCManagedResourceCompilerTool"
	/>
	<Tool
		Name="VCResourceCompilerTool"
	/>
	<Tool
		Name="VCPreLinkEventTool"
	/>
	<Tool
		Name="VCLinkerTool"
	/>
	<Tool
		Name="VCALinkTool"
	/>
	<Tool
		Name="VCManifestTool"
	/>
	<Tool
		Name="VCXDCMakeTool"
	/>
	<Tool
		Name="VCBscMakeTool"
	/>
	<Tool
		Name="VCFxCopTool"
	/>
	<Tool
		Name="VCAppVerifierTool"
	/>
	<Tool
		Name="VCWebDeploymentTool"
	/>
	<Tool
		Name="VCPostBuildEventTool"
	/>
</Configuration>"""

MAIN_VS_PROJECT_SKELETON = r"""<?xml version="1.0" encoding="Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="8.00"
	Name="Gears"
	ProjectGUID="{B7B8AE56-AB05-409B-A818-F57B208E193D}"
	RootNamespace="Gears"
	>
	<Platforms>
		<Platform
			Name="Win32"
		/>
	</Platforms>
	<ToolFiles>
	</ToolFiles>
	<Configurations>
	  {{TARGET_SECTIONS}}
	</Configurations>
	<References>
	</References>
	<Files>
	  {{FILES_SECTION}}
	</Files>
	<Globals>
	</Globals>
</VisualStudioProject>"""
