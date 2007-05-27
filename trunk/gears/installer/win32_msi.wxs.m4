m4_changequote(`^',`^')m4_dnl
<?xml version='1.0' ?>

<!--
Copyright 2007, Google Inc.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
 3. Neither the name of Google Inc. nor the names of its contributors may be
    used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

<Wix xmlns='http://schemas.microsoft.com/wix/2006/wi'>
  <Product Id='$(var.OurProductId)' Name='PRODUCT_FRIENDLY_NAME_UQ'
    Language='1033' Version='PRODUCT_VERSION'
    Manufacturer='Google' UpgradeCode='D91DF85A-1C3B-4d62-914B-DEEEF73AD78C'>
    <Package Id='$(var.OurPackageId)' Description='PRODUCT_FRIENDLY_NAME_UQ'
      Comments='PRODUCT_FRIENDLY_NAME_UQ' Manufacturer='Google'
      InstallerVersion='200' Compressed='yes' />
    <Media Id='1' Cabinet='product.cab' EmbedCab='yes' />
    <Upgrade Id='D91DF85A-1C3B-4d62-914B-DEEEF73AD78C'>
      <UpgradeVersion Property='UPGRADING' OnlyDetect='no'
        Minimum='0.0.0.0' IncludeMinimum='yes'
        Maximum='PRODUCT_VERSION' IncludeMaximum='no' />
      <UpgradeVersion Property='NEWERVERSIONDETECTED' OnlyDetect='yes'
        Minimum='PRODUCT_VERSION' IncludeMinimum='yes' />
    </Upgrade>
    <Directory Id='TARGETDIR' Name='SourceDir'>
      <Directory Id='ProgramFilesFolder' Name='PFiles'>
        <Directory Id='GoogleDir' Name='Google'>
          <Directory Id='ProductDir' Name='PRODUCT_FRIENDLY_NAME_UQ'>

            <Component Id='OurIERegistry' Guid='$(var.OurComponentGUID_IERegistry)'>
              <RegistryValue
                Root='HKLM' Key='Software\Google\Update\Clients\{283EAF47-8817-4c2b-A801-AD1FADFB7BAA}'
                Name='pv' Value="PRODUCT_VERSION"
                Action='write' Type='string' />
              <RegistryValue
                Root='HKLM' Key='Software\Google\Update\Clients\{283EAF47-8817-4c2b-A801-AD1FADFB7BAA}'
                Name='ap' Value=""
                Action='write' Type='string' />
            </Component>

            <Component Id='OurFFRegistry' Guid='$(var.OurComponentGUID_FFRegistry)'>
              <!-- IMPORTANT: the 'Name' field below MUST match our <em:id> value in install.rdf.m4 -->
              <RegistryValue
                Root='HKLM' Key='Software\Mozilla\Firefox\Extensions'
                Name='{000a9d1c-beef-4f90-9363-039d445309b8}' Value='[OurFFDir]'
                Action='write' Type='string' />
            </Component>

            <!-- IMPORTANT: the OurShared* 'Name' fields MUST match the WIN32 paths in /{firefox,ie}/PathUtils.cc -->
            <Directory Id='OurSharedDir' Name='Shared'>
              <Directory Id='OurSharedVersionedDir' Name='PRODUCT_VERSION'>
                <!-- Not used right now -->
              </Directory>
            </Directory>

            <Directory Id='OurIEDir' Name='Internet Explorer'>
              <Directory Id='OurIEVersionedDir' Name='PRODUCT_VERSION'>
                <Component Id='OurIEDirFiles' Guid='$(var.OurComponentGUID_IEFiles)'>
                  <File Id='ie_dll' Name='PRODUCT_SHORT_NAME_UQ.dll' DiskId='1'
                    Source="$(var.OurIEPath)/PRODUCT_SHORT_NAME_UQ.dll" SelfRegCost="1" />
m4_ifdef(^DEBUG^,^m4_dnl
                  <File Id='ie_pdb' Name='PRODUCT_SHORT_NAME_UQ.pdb' DiskId='1'
                    Source="$(var.OurIEPath)/PRODUCT_SHORT_NAME_UQ.pdb" />
^)
                </Component>
              </Directory>
            </Directory>

            <!-- Firefox avoids a versioned dir because the Extension Manager
                 sometimes gets confused and disables the updated version. -->
            <Directory Id='OurFFDir' Name='Firefox'>
              <Component Id='OurFFDirFiles' Guid='$(var.OurComponentGUID_FFDirFiles)'>
                <File Id='ff_install.rdf' Name='install.rdf' DiskId='1'
                  Source="$(var.OurFFPath)/install.rdf" />
                <File Id='ff_chrome.manifest' Name='chrome.manifest' DiskId='1'
                  Source="$(var.OurFFPath)/chrome.manifest" />
              </Component>
              <Directory Id='OurFFComponentsDir' Name='components'>
                <Component Id='OurFFComponentsDirFiles'
                  Guid='$(var.OurComponentGUID_FFComponentsDirFiles)'>
                  <File Id='ff_bootstrap.js' Name='bootstrap.js'
                    DiskId='1' Source="$(var.OurFFPath)/components/bootstrap.js" />
                  <File Id='ff_dll' Name='PRODUCT_SHORT_NAME_UQ.dll' DiskId='1'
                    Source="$(var.OurFFPath)/components/PRODUCT_SHORT_NAME_UQ.dll" />
m4_ifdef(^DEBUG^,^m4_dnl
                  <File Id='ff_pdb' Name='PRODUCT_SHORT_NAME_UQ.pdb' DiskId='1'
                    Source="$(var.OurFFPath)/components/PRODUCT_SHORT_NAME_UQ.pdb" />
^)
                  <File Id='ff_xpt' Name='PRODUCT_SHORT_NAME_UQ.xpt' DiskId='1'
                    Source="$(var.OurFFPath)/components/PRODUCT_SHORT_NAME_UQ.xpt" />
                </Component>
              </Directory>
              <Directory Id='OurFFChromeDir' Name='chrome'>
                <Directory Id='OurFFChromeFilesDir' Name='chromeFiles'>
                  <Directory Id='OurFFContentDir' Name='content'>
                    <Component Id='OurFFContentDirFiles'
                      Guid='$(var.OurComponentGUID_FFContentDirFiles)'>
                      <File Id='ff_button_row_background.gif' Name='button_row_background.gif'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/button_row_background.gif" />
                      <File Id='ff_browser_overlay.js' Name='browser-overlay.js'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/browser-overlay.js" />
                      <File Id='ff_browser_overlay.xul' Name='browser-overlay.xul'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/browser-overlay.xul" />
                      <File Id='ff_html_dialog.css' Name='html_dialog.css'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/html_dialog.css" />
                      <File Id='ff_html_dialog.js' Name='html_dialog.js'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/html_dialog.js" />
                      <File Id='ff_json_noeval.js' Name='json_noeval.js'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/json_noeval.js" />
                      <File Id='ff_permissions_dialog.html' Name='permissions_dialog.html'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/permissions_dialog.html" />
                      <File Id='ff_settings_dialog.html' Name='settings_dialog.html'
                        DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/content/settings_dialog.html" />
                    </Component>
                  </Directory>
                  <Directory Id='OurFFLocaleDir' Name='locale'>
                    <Directory Id='OurFFEnUsDir' Name='en-US'>
                      <Component Id='OurFFEnUsDirFiles'
                        Guid='$(var.OurComponentGUID_FFEnUsDirFiles)'>
                        <File Id='ff_en_US_i18n.dtd' Name='i18n.dtd'
                          DiskId='1' Source="$(var.OurFFPath)/chrome/chromeFiles/locale/en-US/i18n.dtd" />
                      </Component>
                    </Directory>
                  </Directory>
                </Directory>
              </Directory>
              <Directory Id='OurFFLibDir' Name='lib'>
                <Component Id='OurFFLibDirFiles'
                  Guid='$(var.OurComponentGUID_FFLibDirFiles)'>
                  <File Id='ff_updater.js' Name='updater.js'
                    DiskId='1' Source="$(var.OurFFPath)/lib/updater.js" />
                </Component>
              </Directory>
            </Directory>

          </Directory>
        </Directory>
      </Directory>
    </Directory>
    <Feature Id='PRODUCT_SHORT_NAME_UQ' Title='PRODUCT_FRIENDLY_NAME_UQ' Level='1'>
      <ComponentRef Id='OurIERegistry' />
      <ComponentRef Id='OurFFRegistry' />
      <ComponentRef Id='OurIEDirFiles' />
      <ComponentRef Id='OurFFDirFiles' />
      <ComponentRef Id='OurFFComponentsDirFiles' />
      <ComponentRef Id='OurFFLibDirFiles' />
      <ComponentRef Id='OurFFContentDirFiles' />
      <ComponentRef Id='OurFFEnUsDirFiles' />
    </Feature>
    <Condition Message='PRODUCT_FRIENDLY_NAME_UQ has already been updated.'>
      NOT NEWERVERSIONDETECTED
    </Condition>
    <InstallExecuteSequence>
      <RemoveExistingProducts After='InstallValidate'>UPGRADING</RemoveExistingProducts>
    </InstallExecuteSequence>
    <Property Id="UILevel"><![CDATA[1]]></Property>
    <Property Id="ALLUSERS"><![CDATA[1]]></Property>
  </Product>
</Wix>
