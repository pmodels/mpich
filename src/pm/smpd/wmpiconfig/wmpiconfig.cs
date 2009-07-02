#define VERIFY_USING_REGISTRY
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using Microsoft.Win32; // registry reading/writing
using System.Net; // Dns
using System.IO; // File
using System.Diagnostics; // Process
using System.Runtime.InteropServices;
using System.Text; // StringBuilder

namespace wmpiconfig
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class wmpiconfig : System.Windows.Forms.Form
	{
		private bool domain_populated;
		private Color setting_color;
		private Hashtable hash;
		private string smpd;
		private string mpiexec;
		private System.Windows.Forms.Label host_label;
		private System.Windows.Forms.TextBox host_textBox;
		private System.Windows.Forms.Button get_settings_button;
		private System.Windows.Forms.ListView list;
		private System.Windows.Forms.Button apply_button;
		private System.Windows.Forms.Button cancel_button;
		private System.Windows.Forms.ListView hosts_list;
		private System.Windows.Forms.Label domain_label;
		private System.Windows.Forms.Button scan_button;
		private System.Windows.Forms.ColumnHeader HostsHeader;
		private System.Windows.Forms.ColumnHeader SettingsHeader;
		private System.Windows.Forms.ColumnHeader DefaultHeader;
		private System.Windows.Forms.ColumnHeader AvailableHeader;
		private System.Windows.Forms.TextBox output_textBox;
		private System.Windows.Forms.Button ok_button;
		private System.Windows.Forms.ComboBox domain_comboBox;
		private System.Windows.Forms.Button get_hosts_button;
		private System.Windows.Forms.Button apply_all_button;
		private System.Windows.Forms.CheckBox append_checkBox;
		private System.Windows.Forms.CheckBox click_checkBox;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;
		private System.Windows.Forms.Button toggle_button;
		private System.Windows.Forms.ProgressBar scan_progressBar;
		private ToolTip tool_tip;
		private System.Windows.Forms.Button versions_button;
		private System.Windows.Forms.ColumnHeader VersionHeader;
		private Color orig_background;

		public wmpiconfig()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			hash = new Hashtable();
			smpd = get_smpd();
			mpiexec = get_mpiexec();
			setting_color = Color.FromArgb(204,230,230);

			domain_comboBox.Text = Environment.UserDomainName;
			domain_populated = false;
			host_textBox.Text = Environment.MachineName.ToLower();
			hosts_list.Items.Add(host_textBox.Text);

			// set defaults
			hash["log"] = new Setting("log", "", "no", "yes,no");
			hash["logfile"] = new Setting("logfile", "", "none", @"filename (example: c:\temp\smpd.log)");
			hash["channel"] = new Setting("channel", "", "sock", "nemesis,sock,mt,ssm,shm,auto");
			hash["internode_channel"] = new Setting("internode_channel", "", "ssm", "nemesis,sock,mt,ssm");
			hash["phrase"] = new Setting("phrase", "", "", "");
			hash["hosts"] = new Setting("hosts", "", "localhost", "list of hostnames (example: foo bar bazz)");
			hash["max_logfile_size"] = new Setting("max_logfile_size", "", "unlimited", "max number of bytes");
			hash["timeout"] = new Setting("timeout", "", "infinite", "max number of seconds");
			//hash["map_drives"] = new Setting("map_drives", "", "no", "yes,no");
			hash["exitcodes"] = new Setting("exitcodes", "", "no", "yes,no");
			hash["port"] = new Setting("port", "", "8676", "");
			hash["noprompt"] = new Setting("noprompt", "", "no", "yes,no");
			hash["priority"] = new Setting("priority", "", "2:3", "0..4[:0..5] idle, below, normal, above, high[:idle, lowest, below, normal, above, highest]");
			hash["app_path"] = new Setting("app_path", "", "", "path to search for user executables");
			hash["plaintext"] = new Setting("plaintext", "", "no", "yes,no");
			hash["localonly"] = new Setting("localonly", "", "no", "yes,no");
			hash["nocache"] = new Setting("nocache", "", "no", "yes,no");
			hash["delegate"] = new Setting("delegate", "", "no", "yes,no");
			hash["sspi_protect"] = new Setting("sspi_protect", "", "no", "yes,no");

			UpdateHash(get_settings(host_textBox.Text));
			UpdateListBox();

			orig_background = list.BackColor;
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(wmpiconfig));
			this.host_label = new System.Windows.Forms.Label();
			this.host_textBox = new System.Windows.Forms.TextBox();
			this.get_settings_button = new System.Windows.Forms.Button();
			this.list = new System.Windows.Forms.ListView();
			this.SettingsHeader = new System.Windows.Forms.ColumnHeader();
			this.DefaultHeader = new System.Windows.Forms.ColumnHeader();
			this.AvailableHeader = new System.Windows.Forms.ColumnHeader();
			this.apply_button = new System.Windows.Forms.Button();
			this.cancel_button = new System.Windows.Forms.Button();
			this.hosts_list = new System.Windows.Forms.ListView();
			this.HostsHeader = new System.Windows.Forms.ColumnHeader();
			this.domain_label = new System.Windows.Forms.Label();
			this.scan_button = new System.Windows.Forms.Button();
			this.output_textBox = new System.Windows.Forms.TextBox();
			this.ok_button = new System.Windows.Forms.Button();
			this.domain_comboBox = new System.Windows.Forms.ComboBox();
			this.get_hosts_button = new System.Windows.Forms.Button();
			this.apply_all_button = new System.Windows.Forms.Button();
			this.append_checkBox = new System.Windows.Forms.CheckBox();
			this.click_checkBox = new System.Windows.Forms.CheckBox();
			this.toggle_button = new System.Windows.Forms.Button();
			this.scan_progressBar = new System.Windows.Forms.ProgressBar();
			this.versions_button = new System.Windows.Forms.Button();
			this.VersionHeader = new System.Windows.Forms.ColumnHeader();
			this.SuspendLayout();
			// 
			// host_label
			// 
			this.host_label.Location = new System.Drawing.Point(144, 8);
			this.host_label.Name = "host_label";
			this.host_label.Size = new System.Drawing.Size(32, 16);
			this.host_label.TabIndex = 0;
			this.host_label.Text = "Host:";
			// 
			// host_textBox
			// 
			this.host_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.host_textBox.Location = new System.Drawing.Point(176, 8);
			this.host_textBox.Name = "host_textBox";
			this.host_textBox.Size = new System.Drawing.Size(312, 20);
			this.host_textBox.TabIndex = 1;
			this.host_textBox.Text = "localhost";
			// 
			// get_settings_button
			// 
			this.get_settings_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.get_settings_button.Location = new System.Drawing.Point(408, 32);
			this.get_settings_button.Name = "get_settings_button";
			this.get_settings_button.Size = new System.Drawing.Size(80, 23);
			this.get_settings_button.TabIndex = 2;
			this.get_settings_button.Text = "&Get Settings";
			this.get_settings_button.Click += new System.EventHandler(this.get_settings_button_Click);
			// 
			// list
			// 
			this.list.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.list.CheckBoxes = true;
			this.list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																				   this.SettingsHeader,
																				   this.DefaultHeader,
																				   this.AvailableHeader});
			this.list.FullRowSelect = true;
			this.list.GridLines = true;
			this.list.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.list.Location = new System.Drawing.Point(144, 64);
			this.list.Name = "list";
			this.list.Size = new System.Drawing.Size(344, 376);
			this.list.TabIndex = 3;
			this.list.View = System.Windows.Forms.View.Details;
			this.list.ItemActivate += new System.EventHandler(this.list_ItemActivate);
			this.list.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.list_AfterLabelEdit);
			this.list.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.list_ItemCheck);
			// 
			// SettingsHeader
			// 
			this.SettingsHeader.Text = "Settings";
			this.SettingsHeader.Width = 132;
			// 
			// DefaultHeader
			// 
			this.DefaultHeader.Text = "Default";
			// 
			// AvailableHeader
			// 
			this.AvailableHeader.Text = "Available options";
			this.AvailableHeader.Width = 122;
			// 
			// apply_button
			// 
			this.apply_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.apply_button.Location = new System.Drawing.Point(248, 448);
			this.apply_button.Name = "apply_button";
			this.apply_button.TabIndex = 4;
			this.apply_button.Text = "&Apply";
			this.apply_button.Click += new System.EventHandler(this.apply_button_Click);
			// 
			// cancel_button
			// 
			this.cancel_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.cancel_button.Location = new System.Drawing.Point(416, 448);
			this.cancel_button.Name = "cancel_button";
			this.cancel_button.TabIndex = 14;
			this.cancel_button.Text = "&Cancel";
			this.cancel_button.Click += new System.EventHandler(this.cancel_button_Click);
			// 
			// hosts_list
			// 
			this.hosts_list.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left)));
			this.hosts_list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																						 this.HostsHeader,
																						 this.VersionHeader});
			this.hosts_list.FullRowSelect = true;
			this.hosts_list.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.hosts_list.Location = new System.Drawing.Point(8, 144);
			this.hosts_list.Name = "hosts_list";
			this.hosts_list.Size = new System.Drawing.Size(128, 328);
			this.hosts_list.TabIndex = 12;
			this.hosts_list.View = System.Windows.Forms.View.Details;
			this.hosts_list.KeyUp += new System.Windows.Forms.KeyEventHandler(this.hosts_list_KeyUp);
			this.hosts_list.SelectedIndexChanged += new System.EventHandler(this.hosts_list_SelectedIndexChanged);
			// 
			// HostsHeader
			// 
			this.HostsHeader.Text = "Hosts";
			this.HostsHeader.Width = 91;
			// 
			// domain_label
			// 
			this.domain_label.Location = new System.Drawing.Point(8, 8);
			this.domain_label.Name = "domain_label";
			this.domain_label.Size = new System.Drawing.Size(48, 16);
			this.domain_label.TabIndex = 7;
			this.domain_label.Text = "Domain:";
			// 
			// scan_button
			// 
			this.scan_button.Location = new System.Drawing.Point(8, 80);
			this.scan_button.Name = "scan_button";
			this.scan_button.Size = new System.Drawing.Size(80, 23);
			this.scan_button.TabIndex = 9;
			this.scan_button.Text = "&Scan Hosts";
			this.scan_button.Click += new System.EventHandler(this.scan_button_Click);
			// 
			// output_textBox
			// 
			this.output_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.output_textBox.Location = new System.Drawing.Point(144, 32);
			this.output_textBox.Multiline = true;
			this.output_textBox.Name = "output_textBox";
			this.output_textBox.ReadOnly = true;
			this.output_textBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.output_textBox.Size = new System.Drawing.Size(256, 32);
			this.output_textBox.TabIndex = 15;
			this.output_textBox.Text = "";
			// 
			// ok_button
			// 
			this.ok_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_button.Location = new System.Drawing.Point(336, 448);
			this.ok_button.Name = "ok_button";
			this.ok_button.Size = new System.Drawing.Size(72, 23);
			this.ok_button.TabIndex = 13;
			this.ok_button.Text = "OK";
			this.ok_button.Click += new System.EventHandler(this.ok_button_Click);
			// 
			// domain_comboBox
			// 
			this.domain_comboBox.Location = new System.Drawing.Point(8, 24);
			this.domain_comboBox.Name = "domain_comboBox";
			this.domain_comboBox.Size = new System.Drawing.Size(112, 21);
			this.domain_comboBox.TabIndex = 6;
			this.domain_comboBox.DropDown += new System.EventHandler(this.domain_comboBox_DropDown);
			// 
			// get_hosts_button
			// 
			this.get_hosts_button.Location = new System.Drawing.Point(8, 48);
			this.get_hosts_button.Name = "get_hosts_button";
			this.get_hosts_button.TabIndex = 7;
			this.get_hosts_button.Text = "Get &Hosts";
			this.get_hosts_button.Click += new System.EventHandler(this.get_hosts_button_Click);
			// 
			// apply_all_button
			// 
			this.apply_all_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.apply_all_button.Location = new System.Drawing.Point(168, 448);
			this.apply_all_button.Name = "apply_all_button";
			this.apply_all_button.TabIndex = 5;
			this.apply_all_button.Text = "Apply All";
			this.apply_all_button.Click += new System.EventHandler(this.apply_all_button_Click);
			// 
			// append_checkBox
			// 
			this.append_checkBox.Appearance = System.Windows.Forms.Appearance.Button;
			this.append_checkBox.Location = new System.Drawing.Point(88, 51);
			this.append_checkBox.Name = "append_checkBox";
			this.append_checkBox.Size = new System.Drawing.Size(16, 16);
			this.append_checkBox.TabIndex = 8;
			this.append_checkBox.Text = "+";
			// 
			// click_checkBox
			// 
			this.click_checkBox.Appearance = System.Windows.Forms.Appearance.Button;
			this.click_checkBox.Location = new System.Drawing.Point(96, 80);
			this.click_checkBox.Name = "click_checkBox";
			this.click_checkBox.Size = new System.Drawing.Size(40, 24);
			this.click_checkBox.TabIndex = 10;
			this.click_checkBox.Text = "click";
			// 
			// toggle_button
			// 
			this.toggle_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.toggle_button.Location = new System.Drawing.Point(144, 448);
			this.toggle_button.Name = "toggle_button";
			this.toggle_button.Size = new System.Drawing.Size(16, 23);
			this.toggle_button.TabIndex = 16;
			this.toggle_button.Text = "^";
			this.toggle_button.Click += new System.EventHandler(this.toggle_button_Click);
			// 
			// scan_progressBar
			// 
			this.scan_progressBar.Location = new System.Drawing.Point(8, 136);
			this.scan_progressBar.Name = "scan_progressBar";
			this.scan_progressBar.Size = new System.Drawing.Size(128, 8);
			this.scan_progressBar.Step = 1;
			this.scan_progressBar.TabIndex = 17;
			// 
			// versions_button
			// 
			this.versions_button.Location = new System.Drawing.Point(8, 112);
			this.versions_button.Name = "versions_button";
			this.versions_button.Size = new System.Drawing.Size(128, 23);
			this.versions_button.TabIndex = 11;
			this.versions_button.Text = "Scan for &Versions";
			this.versions_button.Click += new System.EventHandler(this.versions_button_Click);
			// 
			// VersionHeader
			// 
			this.VersionHeader.Text = "Version";
			// 
			// wmpiconfig
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(496, 483);
			this.Controls.Add(this.versions_button);
			this.Controls.Add(this.scan_progressBar);
			this.Controls.Add(this.toggle_button);
			this.Controls.Add(this.cancel_button);
			this.Controls.Add(this.apply_button);
			this.Controls.Add(this.click_checkBox);
			this.Controls.Add(this.append_checkBox);
			this.Controls.Add(this.apply_all_button);
			this.Controls.Add(this.get_hosts_button);
			this.Controls.Add(this.domain_comboBox);
			this.Controls.Add(this.ok_button);
			this.Controls.Add(this.output_textBox);
			this.Controls.Add(this.host_textBox);
			this.Controls.Add(this.scan_button);
			this.Controls.Add(this.domain_label);
			this.Controls.Add(this.hosts_list);
			this.Controls.Add(this.list);
			this.Controls.Add(this.get_settings_button);
			this.Controls.Add(this.host_label);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MinimumSize = new System.Drawing.Size(504, 256);
			this.Name = "wmpiconfig";
			this.Text = "MPICH2 Configurable Settings";
			this.Load += new System.EventHandler(this.wmpiconfig_Load);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new wmpiconfig());
		}

		internal void PopulateDomainList()
		{
			ComputerBrowser ce;
			string domainName = "";
			string currentDomain = System.Environment.UserDomainName;
			int index = -1;

			Cursor.Current = Cursors.WaitCursor;

			// browse for domains
			ce = new ComputerBrowser(ComputerBrowser.ServerType.SV_TYPE_DOMAIN_ENUM);
			for (int i=0; i<ce.Length; i++)
			{
				domainName = ce[i].Name;
				// Add the name to the dropdown list if it already doesn't exist
				bool found = false;
				foreach (string str in domain_comboBox.Items)
				{
					if (str.ToLower() == domainName.ToLower())
						found = true;
				}
				if (!found)
				{
					domain_comboBox.Items.Add(domainName);
				}
				// Save the index of the current machine's domain so it will be selected by default
				if (domainName.ToLower() == currentDomain.ToLower())
				{
					index = i;
				}
			}
			if (index == -1)
			{
				index = domain_comboBox.Items.Add(currentDomain);
			}
			domain_comboBox.SelectedIndex = index;
			Cursor.Current = Cursors.Default;
		}

		internal string [] GetHostNames()
		{
			ComputerBrowser ce;
			string [] results = null;

			Cursor.Current = Cursors.WaitCursor;

			ce = new ComputerBrowser(domain_comboBox.Text);
			int numServer = ce.Length;

			if (ce.LastError.Length == 0)
			{
				IEnumerator enumerator = ce.GetEnumerator();

				// add the domain text to the dropdown list if it isn't already there
				bool found = false;
				foreach (string str in domain_comboBox.Items)
				{
					if (str == domain_comboBox.Text)
					{
						found = true;
					}
				}
				if (!found)
				{
					domain_comboBox.Items.Add(domain_comboBox.Text);
				}

				results = new string[numServer];
				int i = 0;
				while (enumerator.MoveNext())
				{
					results[i] = ce[i].Name;
					i++;
				}
			}
			else
			{
				output_textBox.Text = "Error \"" + ce.LastError + "\"";
			}

			Cursor.Current = Cursors.Default;
			return results;
		}

		private void UpdateHash(Hashtable h)
		{
			// update or add entries to the internal hash for each key in the input hash
			foreach (string str in h.Keys)
			{
				if (str != "binary") // ignore the smpd binary key because it is machine specific
				{
					if (hash.Contains(str))
					{
						((Setting)hash[str])._value = (string)h[str];
					}
					else
					{
						hash[str] = new Setting(str, (string)h[str], "", "");
					}
				}
			}
			// remove settings from the internal hash for keys not in the input hash
			foreach (string str in hash.Keys)
			{
				if (!h.Contains(str))
				{
					((Setting)hash[str])._value = "";
				}
			}
		}

		private void UpdateListBox()
		{
			int i;
			Hashtable h = new Hashtable();
			// make a hash of the settings already in the listbox
			for (i=0; i<list.Items.Count; i+=2)
			{
				// add the name to the hash
				h.Add(list.Items[i].Text, i+1);
				// clear the current setting
				list.Items[i+1].Text = "";
			}
			// update or add items to the listbox
			foreach (string str in hash.Keys)
			{
				if (h.Contains(str))
				{
					i = (int)h[str];
					list.Items[i].Text = ((Setting)hash[str])._value;
				}
				else
				{
					Setting s = (Setting)hash[str];
					ListViewItem item = list.Items.Add(str);
					item.BackColor = setting_color;
					item = list.Items.Add(s._value);
					item.SubItems.Add(s._default_val);
					item.SubItems.Add(s._available_values);
				}
			}
		}

		delegate void ScanHostDelegate(string host);
		private void ScanHost(string host)
		{
			bool success;
			string result = host + "\r\n";
			Hashtable h;
			h = get_settings(host);
			success = !(h.Contains("error") || h.Count == 0);

			SetHostBackgroundDelegate sd = new SetHostBackgroundDelegate(set_host_background);
			object [] sd_list = new object[2] { host, success };
			Invoke(sd, sd_list);
			//set_host_background(host, success);

			foreach (string key in h.Keys)
			{
				result = result + key + " = " + h[key] + "\r\n";
			}
			UpdateHostScanResultDelegate rd = new UpdateHostScanResultDelegate(UpdateHostScanResult);
			object [] list = new object[1] { result };
			Invoke(rd, list);
		}

		delegate void UpdateHostScanResultDelegate(string result);
		private void UpdateHostScanResult(string result)
		{
			output_textBox.AppendText(result);
			scan_progressBar.PerformStep();
		}

		delegate void ScanHostVersionDelegate(string host);
		private void ScanHostVersion(string host)
		{
			string version;
			version = get_version(host);

			UpdateHostScanVersionResultDelegate rd = new UpdateHostScanVersionResultDelegate(UpdateHostScanVersionResult);
			object [] list = new object[2] { host, version };
			Invoke(rd, list);
		}

		delegate void UpdateHostScanVersionResultDelegate(string host, string version);
		private void UpdateHostScanVersionResult(string host, string version)
		{
			add_host_version_to_list(host, version);
			scan_progressBar.PerformStep();
		}

		private void scan_button_Click(object sender, System.EventArgs e)
		{
			// Get the versions
			versions_button_Click(sender, e);
			// Then get the rest of the details
			scan_progressBar.Value = 0;
			scan_progressBar.Maximum = hosts_list.Items.Count;
			output_textBox.Text = "";
			foreach (ListViewItem item in hosts_list.Items)
			{
				item.BackColor = orig_background;
				ScanHostDelegate shd = new ScanHostDelegate(ScanHost);
				shd.BeginInvoke(item.Text, null, null);
			}
		}

		private void VerifyEncryptedPasswordExists()
		{
			try
			{
				bool popup = true;
				string mpiexec = get_mpiexec();
#if VERIFY_USING_REGISTRY
				// Check the registry for the encrypted password
				// This code will have to be kept synchronized with the smpd code
				// The advantage of this approach is that the credentials don't have to be valid on the local host
				RegistryKey key = Registry.CurrentUser.OpenSubKey(@"Software\MPICH");
				if (key != null)
				{
					// check to see that an encrypted password for the current user exists
					object obj = key.GetValue("smpdPassword");
					key.Close();
					if (obj != null)
					{
						popup = false;
					}
				}
#else
				// Or run "mpiexec -validate" and check the output for SUCCESS
				// This code will last longer because it doesn't rely on known information about the smpd implementation.
				// The disadvantage of this code is that the user credentials have to be valid on the local host.
				Process p1 = new Process();
				p1.StartInfo.RedirectStandardOutput = true;
				p1.StartInfo.RedirectStandardError = true;
				p1.StartInfo.UseShellExecute = false;
				p1.StartInfo.CreateNoWindow = true;
				p1.StartInfo.FileName = mpiexec;
				p1.StartInfo.Arguments = "-validate";
				p1.Start();
				string output = p1.StandardOutput.ReadToEnd() + p1.StandardError.ReadToEnd();
				p1.WaitForExit();
				p1.Close();
				if (output.IndexOf("SUCCESS") != -1)
				{
					popup = false;
				}
#endif
				if (popup)
				{
					string wmpiregister;
					wmpiregister = mpiexec.Replace("mpiexec.exe", "wmpiregister.exe");
					Process p = new Process();
					p.StartInfo.FileName = wmpiregister;
					p.Start();
					p.WaitForExit();
				}
			}
			catch (Exception)
			{
			}
		}

		private Hashtable get_settings(string host)
		{
			string option, val;
			Hashtable hash = new Hashtable();

			VerifyEncryptedPasswordExists();

			Process p = new Process();
			p.StartInfo.FileName = mpiexec;
			p.StartInfo.Arguments = string.Format("-timeout 20 -noprompt -path {{SMPD_PATH}} -n 1 -host {0} smpd.exe -enumerate", host);
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;

			//MessageBox.Show("About to launch: " + p.StartInfo.FileName + " " + p.StartInfo.Arguments);
			try
			{
				p.Start();
				while ((option = p.StandardOutput.ReadLine()) != null)
				{
					val = p.StandardOutput.ReadLine();
					if (val == null)
						break;
					hash.Add(option, val);
				}
				if (!p.WaitForExit(20000))
				{
					p.Kill();
				}
				if (p.ExitCode != 0)
				{
					hash.Clear();
				}
				p.Close();
			}
			catch (Exception)
			{
				hash.Clear();
				hash.Add("error", host + ": Unable to detect the settings");
			}
			if (hash.Count == 0)
			{
				hash.Add("error", host + ": MPICH2 not installed or unable to query the host");
			}
			return hash;
		}

		private string get_version(string host)
		{
			string version = "";

			Process p = new Process();
			p.StartInfo.FileName = smpd;
			p.StartInfo.Arguments = string.Format("-version {0}", host);
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;

			//MessageBox.Show("About to launch: " + p.StartInfo.FileName + " " + p.StartInfo.Arguments);
			try
			{
				p.Start();
				version = p.StandardOutput.ReadToEnd();
				if (!p.WaitForExit(20000))
				{
					p.Kill();
				}
				if (p.ExitCode != 0)
				{
					version = "";
				}
				p.Close();
			}
			catch (Exception)
			{
			}
			return version.Trim();
		}

		private void set_settings(string host, Hashtable h)
		{
			StringBuilder str;
			string val;

			if (h.Keys.Count < 1)
			{
				// nothing to set
				return;
			}

			VerifyEncryptedPasswordExists();

			str = new StringBuilder("-timeout 20 -noprompt -path {SMPD_PATH} -n 1 -host ");
			str.AppendFormat("{0} smpd.exe", host);
			foreach (string key in h.Keys)
			{
				val = (string)h[key];
				if (val.IndexOf(' ') != -1 || val.Length == 0)
				{
					str.AppendFormat(" -set {0} \"{1}\"", key, val);
				}
				else
				{
					str.AppendFormat(" -set {0} {1}", key, val);
				}
			}
			//output_textBox.Text = str.ToString();

			Process p = new Process();
			p.StartInfo.FileName = mpiexec;
			p.StartInfo.Arguments = str.ToString();
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;

			//MessageBox.Show("About to launch: " + p.StartInfo.FileName + " " + p.StartInfo.Arguments);
			try
			{
				output_textBox.AppendText(host + "\r\n");
				p.Start();
				while ((val = p.StandardOutput.ReadLine()) != null)
				{
					output_textBox.AppendText(val + "\r\n");
				}
				if (!p.WaitForExit(20000))
				{
					p.Kill();
				}
				p.Close();
			}
			catch (Exception)
			{
			}
		}

		private Hashtable get_setttings_hash()
		{
			int i;
			Hashtable h = new Hashtable();
			for (i=0; i<list.Items.Count; i+=2)
			{
				if (list.Items[i].Checked)
				{
					h[list.Items[i].Text] = list.Items[i+1].Text;
				}
			}
			return h;
		}

		private string get_mpiexec()
		{
			string mpiexec = "";
			object obj;
			try
			{
				RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH2");
				if (key != null)
				{
					obj = key.GetValue("Path");
					key.Close();
					if (obj != null)
					{
						mpiexec = obj.ToString();
						if (mpiexec.EndsWith(@"\"))
						{
							mpiexec = mpiexec + @"bin\mpiexec.exe";
						}
						else
						{
							mpiexec = mpiexec + @"\bin\mpiexec.exe";
						}
						if (!File.Exists(mpiexec))
						{
							mpiexec = "";
						}
					}
				}
				if (mpiexec == "")
				{
					key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
					if (key != null)
					{
						obj = key.GetValue("binary");
						key.Close();
						if (obj != null)
						{
							mpiexec = obj.ToString().Replace("smpd.exe", "mpiexec.exe");
							if (!File.Exists(mpiexec))
							{
								mpiexec = "";
							}
						}
					}
				}
				if (mpiexec == "")
				{
					mpiexec = "mpiexec.exe";
				}
				mpiexec = mpiexec.Trim();
				/*
				if (mpiexec.IndexOf(' ') != -1)
				{
					mpiexec = "\"" + mpiexec + "\"";
				}
				*/
			}
			catch (Exception)
			{
				mpiexec = "mpiexec.exe";
			}
			return mpiexec;
		}

		private string get_smpd()
		{
			string smpd = "";
			object obj;
			try
			{
				RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH2");
				if (key != null)
				{
					obj = key.GetValue("Path");
					key.Close();
					if (obj != null)
					{
						smpd = obj.ToString();
						if (smpd.EndsWith(@"\"))
						{
							smpd = smpd + @"bin\smpd.exe";
						}
						else
						{
							smpd = smpd + @"\bin\smpd.exe";
						}
						if (!File.Exists(smpd))
						{
							smpd = "";
						}
					}
				}
				if (smpd == "")
				{
					key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
					if (key != null)
					{
						obj = key.GetValue("binary");
						key.Close();
						if (obj != null)
						{
							smpd = obj.ToString();
							if (!File.Exists(smpd))
							{
								smpd = "";
							}
						}
					}
				}
				if (smpd == "")
				{
					smpd = "smpd.exe";
				}
				smpd = smpd.Trim();
				/*
				if (smpd.IndexOf(' ') != -1)
				{
					smpd = "\"" + smpd + "\"";
				}
				*/
			}
			catch (Exception)
			{
				smpd = "smpd.exe";
			}
			return smpd;
		}

		private string get_value(string key)
		{
			object obj;
			try
			{
				RegistryKey regkey = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
				if (regkey != null)
				{
					obj = regkey.GetValue(key);
					regkey.Close();
					if (obj != null)
					{
						return obj.ToString();
					}
				}
			}
			catch (Exception)
			{
			}
			return "";
		}

		private void get_settings_button_Click(object sender, System.EventArgs e)
		{
			bool success;
			Hashtable h;
			Cursor.Current = Cursors.WaitCursor;
			output_textBox.Text = "";
			h = get_settings(host_textBox.Text);
			UpdateHash(h);
			UpdateListBox();
			add_host_to_list(host_textBox.Text);
			success = !(h.Contains("error") || h.Count == 0);
			set_host_background(host_textBox.Text, success);
			Cursor.Current = Cursors.Default;
		}

		private void add_host_to_list(string host)
		{
			bool found = false;
			foreach (ListViewItem item in hosts_list.Items)
			{
				if (String.Compare(item.Text, host, true) == 0)
					found = true;
			}
			if (!found)
			{
				hosts_list.Items.Add(host);
			}
		}

		private void add_host_version_to_list(string host, string version)
		{
			foreach (ListViewItem item in hosts_list.Items)
			{
				if (String.Compare(item.Text, host, true) == 0)
				{
					if (item.SubItems.Count == 2)
						item.SubItems[1].Text = version;
					else
						item.SubItems.Add(version);
					return;
				}
			}
		}

		delegate void SetHostBackgroundDelegate(string host, bool success);
		private void set_host_background(string host, bool success)
		{
			foreach (ListViewItem item in hosts_list.Items)
			{
				if (String.Compare(item.Text, host, true) == 0)
				{
					if (success)
                        item.BackColor = Color.GreenYellow;
					else
						item.BackColor = Color.LightGray;
				}
			}
		}

		private void apply_button_Click(object sender, System.EventArgs e)
		{
			Hashtable h;

			Cursor.Current = Cursors.WaitCursor;
			output_textBox.Clear();
			h = get_setttings_hash();
			set_settings(host_textBox.Text, h);
			Cursor.Current = Cursors.Default;
		}

		private void apply_all_button_Click(object sender, System.EventArgs e)
		{
			Hashtable h;

			Cursor.Current = Cursors.WaitCursor;
			output_textBox.Clear();
			h = get_setttings_hash();
			scan_progressBar.Value = 0;
			scan_progressBar.Maximum = hosts_list.Items.Count;
			foreach (ListViewItem item in hosts_list.Items)
			{
				set_settings(item.Text, h);
				scan_progressBar.PerformStep();
			}
			Cursor.Current = Cursors.Default;
		}

		private void cancel_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void list_ItemActivate(object sender, System.EventArgs e)
		{
			int index;
			if (list.SelectedIndices.Count > 0)
			{
				index = list.SelectedIndices[0];
				if ((index & 0x1) == 0)
				{
					index++;
				}
				// turn on and begin editing the value field
				list.LabelEdit = true;
				list.Items[index].BeginEdit();
			}
		}

		private void list_AfterLabelEdit(object sender, System.Windows.Forms.LabelEditEventArgs e)
		{
			// turn off editing after a field has been modified to prevent the name from being modified
			list.LabelEdit = false;
			list.Items[e.Item].Checked = true;
		}

		private void ok_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void hosts_list_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			if (hosts_list.SelectedItems.Count > 0)
			{
				host_textBox.Text = hosts_list.SelectedItems[0].Text;
				if (click_checkBox.Checked)
				{
					get_settings_button_Click(null, null);
				}
				if (hosts_list.SelectedItems[0].BackColor == Color.LightGray)
				{
					list.BackColor = Color.LightGray;
					list.Enabled = false;
				}
				else
				{
					list.BackColor = orig_background;
					list.Enabled = true;
				}
			}
			else
			{
				list.BackColor = orig_background;
				list.Enabled = true;
			}
		}

		private void domain_comboBox_DropDown(object sender, System.EventArgs e)
		{
			if (!domain_populated)
			{
				PopulateDomainList();
				domain_populated = true;
			}
		}

		private void get_hosts_button_Click(object sender, System.EventArgs e)
		{
			string [] hosts;
			if (domain_comboBox.Text.ToLower() == Environment.MachineName.ToLower())
			{
				hosts = new string[1] { Environment.MachineName };
			}
			else
			{
				hosts = GetHostNames();
			}
			if (hosts != null)
			{
				if (!append_checkBox.Checked)
				{
					hosts_list.Items.Clear();
				}
				// FIXME: This is an N^2 algorithm.
				// It probably won't slow down until there are hundreds of hosts available.
				// A Hashtable should be used instead.
				foreach (string s in hosts)
				{
					bool found = false;
					foreach (ListViewItem item in hosts_list.Items)
					{
						if (item.Text.ToLower() == s.ToLower())
							found = true;
					}
					if (!found)
					{
						hosts_list.Items.Add(s);
					}
				}
			}
		}

		private void wmpiconfig_Load(object sender, System.EventArgs e)
		{
			tool_tip = new ToolTip();

			tool_tip.SetToolTip(append_checkBox, "Append hosts to the list");
			tool_tip.SetToolTip(click_checkBox, "Get settings when a host name is selected");
			tool_tip.SetToolTip(apply_button, "Apply the checked settings to the host in the host edit box");
			tool_tip.SetToolTip(apply_all_button, "Apply the checked settings to all the hosts in the host list");
			tool_tip.SetToolTip(scan_button, "Retrieve the settings from the hosts in the host list");
			tool_tip.SetToolTip(get_hosts_button, "Get the host names from the specified domain");
			tool_tip.SetToolTip(toggle_button, "Check or uncheck all the checked settings");
			tool_tip.SetToolTip(versions_button, "Retrieve the version from the hosts in the host list");
		}

		bool check_recursed = false;
		private void list_ItemCheck(object sender, System.Windows.Forms.ItemCheckEventArgs e)
		{
			int other_index;
			if (!check_recursed)
			{
				check_recursed = true;
				// Each setting takes up two listbox entries so check and uncheck them together.
				// Get the index of the sister checkbox.
				other_index = ((e.Index & 0x1) == 0) ? e.Index + 1 : e.Index - 1;
				list.Items[other_index].Checked = (e.NewValue == CheckState.Checked);
				check_recursed = false;
			}
		}

		/// <summary>
		/// Toggle all the check boxes except the "phrase" and "port" options.
		/// Those options must be selected manually to make their setting more intentional.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		bool checkuncheck = true;
		private void toggle_button_Click(object sender, System.EventArgs e)
		{
			int index = -1;
			int port_index = -1;
			int error_index = -1;
			bool phrase_checked = false;
			bool port_checked = false;
			check_recursed = true;
			foreach (ListViewItem item in list.Items)
			{
				if (item.Text == "phrase")
				{
					// save the index of the item following the phrase item to reset it afterwards
					index = item.Index + 1;
					phrase_checked = item.Checked;
				}
				else if (item.Text == "port")
				{
					port_index = item.Index + 1;
					port_checked = item.Checked;
				}
				else if (item.Text == "error")
				{
					error_index = item.Index + 1;
				}
				else
				{
					item.Checked = checkuncheck; //!item.Checked;
				}
			}
			if (index != -1)
			{
				// reset the phrase value
				list.Items[index].Checked = phrase_checked;
			}
			if (port_index != -1)
			{
				// reset the port value
				list.Items[port_index].Checked = port_checked;
			}
			if (error_index != -1)
			{
				// remove the error item check
				list.Items[error_index].Checked = false;
			}
			checkuncheck = !checkuncheck;
			toggle_button.Text = checkuncheck ? "x" : "o";
			check_recursed = false;
		}

		private void hosts_list_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
		{
			if (e.KeyValue == 46) // FIXME: Is there a generic way to determine the delete key?
			{
				// remove the host from the list
				if (hosts_list.FocusedItem != null)
				{
					hosts_list.Items.Remove(hosts_list.FocusedItem);
				}
			}
		}

		private void versions_button_Click(object sender, System.EventArgs e)
		{
			scan_progressBar.Value = 0;
			scan_progressBar.Maximum = hosts_list.Items.Count;
			output_textBox.Text = "";
			foreach (ListViewItem item in hosts_list.Items)
			{
				ScanHostVersionDelegate shd = new ScanHostVersionDelegate(ScanHostVersion);
				shd.BeginInvoke(item.Text, null, null);
			}
		}
	}

	public class Setting
	{
		public string _name;
		public string _value;
		public string _default_val;
		public string _available_values;

		public Setting()
		{
		}

		public Setting(string name, string val, string default_val, string available_values)
		{
			_name = name;
			_value = val;
			_default_val = default_val;
			_available_values = available_values;
		}
	}

	public class ComputerBrowser : IEnumerable, IDisposable
	{
		#region "Server type enumeration"
		[FlagsAttribute]
		public enum ServerType : uint
		{
			SV_TYPE_WORKSTATION       = 0x00000001, // All workstations
			SV_TYPE_SERVER            = 0x00000002, // All computers that have the server service running
			SV_TYPE_SQLSERVER         = 0x00000004, // Any server running Microsoft SQL Server
			SV_TYPE_DOMAIN_CTRL       = 0x00000008, // Primary domain controller
			SV_TYPE_DOMAIN_BAKCTRL    = 0x00000010, // Backup domain controller
			SV_TYPE_TIME_SOURCE       = 0x00000020, // Server running the Timesource service
			SV_TYPE_AFP               = 0x00000040, // Apple File Protocol servers
			SV_TYPE_NOVELL            = 0x00000080, // Novell servers
			SV_TYPE_DOMAIN_MEMBER     = 0x00000100, // LAN Manager 2.x domain member
			SV_TYPE_PRINTQ_SERVER     = 0x00000200, // Server sharing print queue
			SV_TYPE_DIALIN_SERVER     = 0x00000400, // Server running dial-in service
			SV_TYPE_XENIX_SERVER      = 0x00000800, // Xenix server
			SV_TYPE_NT                = 0x00001000, // Windows NT workstation or server
			SV_TYPE_WFW               = 0x00002000, // Server running Windows for Workgroups
			SV_TYPE_SERVER_MFPN       = 0x00004000, // Microsoft File and Print for NetWare
			SV_TYPE_SERVER_NT         = 0x00008000, // Server that is not a domain controller
			SV_TYPE_POTENTIAL_BROWSER = 0x00010000, // Server that can run the browser service
			SV_TYPE_BACKUP_BROWSER    = 0x00020000, // Server running a browser service as backup
			SV_TYPE_MASTER_BROWSER    = 0x00040000, // Server running the master browser service
			SV_TYPE_DOMAIN_MASTER     = 0x00080000, // Server running the domain master browser
			SV_TYPE_WINDOWS           = 0x00400000, // Windows 95 or later
			SV_TYPE_DFS               = 0x00800000, // Root of a DFS tree
			SV_TYPE_TERMINALSERVER    = 0x02000000, // Terminal Server
			SV_TYPE_CLUSTER_NT        = 0x01000000, // Server clusters available in the domain
			SV_TYPE_CLUSTER_VS_NT     = 0x04000000, // Cluster virtual servers available in the domain
			SV_TYPE_DCE               = 0x10000000, // IBM DSS (Directory and Security Services) or equivalent
			SV_TYPE_ALTERNATE_XPORT   = 0x20000000, // Return list for alternate transport
			SV_TYPE_LOCAL_LIST_ONLY   = 0x40000000, // Return local list only
			SV_TYPE_DOMAIN_ENUM		  = 0x80000000  // Lists available domains
		}
		#endregion
		
		// Server information structure
		[StructLayoutAttribute(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
		internal struct SERVER_INFO_101
		{
			public int    sv101_platform_id;
			public string sv101_name;
			public int    sv101_version_major;
			public int    sv101_version_minor;
			public int    sv101_type;
			public string sv101_comment;
		}

		// enumerates network computers
		[DllImport("Netapi32", CharSet=CharSet.Unicode)]
		private static extern int NetServerEnum( 
			string servername,		// must be null
			int level,				// 100 or 101
			out IntPtr bufptr,		// pointer to buffer receiving data
			int prefmaxlen,			// max length of returned data
			out int entriesread,	// num entries read
			out int totalentries,	// total servers + workstations
			uint servertype,		// server type filter
			[MarshalAs(UnmanagedType.LPWStr)]
			string domain,			// domain to enumerate
			IntPtr resume_handle );
   
		// Frees buffer created by NetServerEnum
		[DllImport("netapi32.dll")]
		private extern static int NetApiBufferFree(IntPtr buf);

		// Constants
		private const int ERROR_ACCESS_DENIED = 5;
		private const int ERROR_MORE_DATA = 234;
		private const int ERROR_NO_SERVERS = 6118;

		private NetworkComputers[] _computers;
		private string _lastError = "";

		public ComputerBrowser(ServerType serverType, string domain) : this(UInt32.Parse(Enum.Format(typeof(ServerType), serverType, "x"), System.Globalization.NumberStyles.HexNumber), domain)
		{
		}
		public ComputerBrowser(ServerType serverType) : this(UInt32.Parse(Enum.Format(typeof(ServerType), serverType, "x"), System.Globalization.NumberStyles.HexNumber), null)
		{
		}
		public ComputerBrowser(string domainName) : this(0xFFFFFFFF, domainName)
		{
		}

		/// <summary>
		/// Enumerate the hosts of type serverType in the specified domain
		/// </summary>
		/// <param name="serverType">Server type filter</param>
		/// <param name="domain">The domain to search for computers in</param>
		public ComputerBrowser(uint serverType, string domainName)
		{			
			int entriesread;  // number of entries actually read
			int totalentries; // total visible servers and workstations
			int result;		  // result of the call to NetServerEnum

			// Pointer to buffer that receives the data
			IntPtr pBuf = IntPtr.Zero;
			Type svType = typeof(SERVER_INFO_101);

			// structure containing info about the server
			SERVER_INFO_101 si;

			try
			{
				result = NetServerEnum(null, 101, out pBuf, -1, out entriesread, out totalentries, serverType, domainName, IntPtr.Zero);

				// Successful?
				if(result != 0) 
				{
					switch (result)
					{
						case ERROR_MORE_DATA:
							_lastError = "More data is available";
							break;
						case ERROR_ACCESS_DENIED:
							_lastError = "Access was denied";
							break;
						case ERROR_NO_SERVERS:
							_lastError = "No servers available for this domain";
							break;
						default:
							_lastError = "Unknown error code "+result;
							break;
					}					
					return;
				}
				else
				{
					_computers = new NetworkComputers[entriesread];

					int tmp = (int)pBuf;
					for(int i = 0; i < entriesread; i++ )
					{
						// fill our struct
						si = (SERVER_INFO_101)Marshal.PtrToStructure((IntPtr)tmp, svType);
						_computers[i] = new NetworkComputers(si);

						// next struct
						tmp += Marshal.SizeOf(svType);
					}
				}
			}
			finally
			{
				// free the buffer
				NetApiBufferFree(pBuf); 
				pBuf = IntPtr.Zero;
			}
		}

		/// <summary>
		/// Total computers in the collection
		/// </summary>
		public int Length
		{
			get
			{
				if(_computers!=null)
				{
					return _computers.Length;
				}
				else
				{
					return 0;
				}
			}
		}
		
		/// <summary>
		/// Last error message
		/// </summary>
		public string LastError
		{
			get { return _lastError; }
		}
		
		/// <summary>
		/// Obtains the enumerator for ComputerEnumerator class
		/// </summary>
		/// <returns>IEnumerator</returns>
		public IEnumerator GetEnumerator()
		{
			return new ComputerEnumerator(_computers);		
		}

		// cleanup
		public void Dispose()
		{
			_computers = null;	
		}
   
		public NetworkComputers this[int item]
		{
			get 
			{ 
				return _computers[item];
			}
		}

		// holds computer info.
		public struct NetworkComputers
		{
			ComputerBrowser.SERVER_INFO_101 _computerinfo;
			internal NetworkComputers(ComputerBrowser.SERVER_INFO_101 info)
			{
				_computerinfo = info;
			}

			/// <summary>
			/// Name of computer
			/// </summary>
			public string Name
			{
				get { return _computerinfo.sv101_name; }
			}
			/// <summary>
			/// Server comment
			/// </summary>
			public string Comment
			{
				get { return _computerinfo.sv101_comment; }
			}
			/// <summary>
			/// Major version number of OS
			/// </summary>
			public int VersionMajor
			{
				get { return _computerinfo.sv101_version_major; }
			}
			/// <summary>
			/// Minor version number of OS
			/// </summary>
			public int VersionMinor
			{
				get { return _computerinfo.sv101_version_minor; }
			}
		}

		/// <summary>
		/// Enumerates the collection of computers
		/// </summary>
		public class ComputerEnumerator : IEnumerator
		{
			private NetworkComputers[] aryComputers;
			private int indexer;

			internal ComputerEnumerator(NetworkComputers[] networkComputers)
			{
				aryComputers = networkComputers;
				indexer = -1;
			}

			public void Reset()
			{
				indexer = -1;
			}

			public object Current
			{
				get
				{
					return aryComputers[indexer];
				}
			}

			public bool MoveNext()
			{
				if (aryComputers == null)
				{
					return false;
				}
				if (indexer < aryComputers.Length)
				{
					indexer++;
				}
				return (!(indexer == aryComputers.Length));
			}
		}
	}
}
