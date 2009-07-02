using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using Microsoft.Win32;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace wmpiexec
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class wmpiexec : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label application_label;
		private System.Windows.Forms.Button application_browse_button;
		private System.Windows.Forms.Label nproc_label;
		private System.Windows.Forms.NumericUpDown nproc_numericUpDown;
		private System.Windows.Forms.Label wdir_label;
		private System.Windows.Forms.TextBox wdir_textBox;
		private System.Windows.Forms.Button wdir_browse_button;
		private System.Windows.Forms.Label hosts_label;
		private System.Windows.Forms.TextBox hosts_textBox;
		private System.Windows.Forms.Button hosts_reset_button;
		private System.Windows.Forms.Label configfile_label;
		private System.Windows.Forms.TextBox configfile_textBox;
		private System.Windows.Forms.Button configfile_browse_button;
		private System.Windows.Forms.Label mpich1_label;
		private System.Windows.Forms.TextBox mpich1_textBox;
		private System.Windows.Forms.Button mpich1_browse_button;
		private System.Windows.Forms.Label env_label;
		private System.Windows.Forms.TextBox env_textBox;
		private System.Windows.Forms.Label drive_map_label;
		private System.Windows.Forms.TextBox drive_map_textBox;
		private System.Windows.Forms.Label channel_label;
		private System.Windows.Forms.ComboBox channel_comboBox;
		private System.Windows.Forms.Button save_job_button;
		private System.Windows.Forms.Button load_job_button;
		private System.Windows.Forms.Button execute_button;
		private System.Windows.Forms.Button break_button;
		private System.Windows.Forms.RichTextBox output_richTextBox;
		private System.Windows.Forms.ComboBox application_comboBox;
		private System.Windows.Forms.Button show_command_button;
		private System.Windows.Forms.TextBox command_line_textBox;
		private System.Windows.Forms.RadioButton application_radioButton;
		private System.Windows.Forms.RadioButton configfile_radioButton;
		private System.Windows.Forms.RadioButton mpich1_radioButton;
		private System.Windows.Forms.CheckBox show_bottom_checkBox;
		private System.Windows.Forms.TextBox extra_options_textBox;
		private System.Windows.Forms.Label extra_options_label;
		private System.Windows.Forms.CheckBox popup_checkBox;
		private System.Windows.Forms.Button jumpshot_button;
		private System.Windows.Forms.CheckBox log_checkBox;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		private RunCommandDelegate run_command;
		private IAsyncResult last_execute_result;
		private Process process;
		private string thread_command;
		private string thread_mpiexec_command;
		private string thread_mpiexec_command_args;
		private bool thread_popup;
		private TextReader thread_output_stream;
		private TextReader thread_error_stream;
		private TextWriter thread_input_stream;
		delegate void RunCommandDelegate(string command, string mpiexec_command, string mpiexec_command_args, bool popup);
		delegate void ReadOutputDelegate(TextReader stream);
		delegate void ReadErrorDelegate(TextReader stream);
		delegate void WriteInputDelegate(TextWriter stream);
		delegate void AppendTextDelegate(string str);
		delegate void SetProcessDelegate(Process p);
		delegate void ResetExecuteButtonsDelegate();

		private string mpiexec_command, mpiexec_command_args;
		private int expanded_dialog_difference;

		public wmpiexec()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			expanded_dialog_difference = mpich1_browse_button.Bottom - show_bottom_checkBox.Bottom + 8;
			//MessageBox.Show(string.Format("difference = {0}", mpich1_browse_button.Bottom - show_bottom_checkBox.Bottom));

			run_command = null;
			last_execute_result = null;
			UpdateExtraControls(show_bottom_checkBox.Checked);
			EnableApplicationControls();
			DisableConfigfileControls();
			DisableMPICH1Controls();
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(wmpiexec));
			this.application_label = new System.Windows.Forms.Label();
			this.application_browse_button = new System.Windows.Forms.Button();
			this.nproc_label = new System.Windows.Forms.Label();
			this.nproc_numericUpDown = new System.Windows.Forms.NumericUpDown();
			this.wdir_label = new System.Windows.Forms.Label();
			this.wdir_textBox = new System.Windows.Forms.TextBox();
			this.wdir_browse_button = new System.Windows.Forms.Button();
			this.hosts_label = new System.Windows.Forms.Label();
			this.hosts_textBox = new System.Windows.Forms.TextBox();
			this.hosts_reset_button = new System.Windows.Forms.Button();
			this.configfile_label = new System.Windows.Forms.Label();
			this.configfile_textBox = new System.Windows.Forms.TextBox();
			this.configfile_browse_button = new System.Windows.Forms.Button();
			this.mpich1_label = new System.Windows.Forms.Label();
			this.mpich1_textBox = new System.Windows.Forms.TextBox();
			this.mpich1_browse_button = new System.Windows.Forms.Button();
			this.env_label = new System.Windows.Forms.Label();
			this.env_textBox = new System.Windows.Forms.TextBox();
			this.drive_map_label = new System.Windows.Forms.Label();
			this.drive_map_textBox = new System.Windows.Forms.TextBox();
			this.channel_label = new System.Windows.Forms.Label();
			this.channel_comboBox = new System.Windows.Forms.ComboBox();
			this.save_job_button = new System.Windows.Forms.Button();
			this.load_job_button = new System.Windows.Forms.Button();
			this.execute_button = new System.Windows.Forms.Button();
			this.break_button = new System.Windows.Forms.Button();
			this.output_richTextBox = new System.Windows.Forms.RichTextBox();
			this.application_comboBox = new System.Windows.Forms.ComboBox();
			this.show_command_button = new System.Windows.Forms.Button();
			this.command_line_textBox = new System.Windows.Forms.TextBox();
			this.application_radioButton = new System.Windows.Forms.RadioButton();
			this.configfile_radioButton = new System.Windows.Forms.RadioButton();
			this.mpich1_radioButton = new System.Windows.Forms.RadioButton();
			this.show_bottom_checkBox = new System.Windows.Forms.CheckBox();
			this.extra_options_textBox = new System.Windows.Forms.TextBox();
			this.extra_options_label = new System.Windows.Forms.Label();
			this.popup_checkBox = new System.Windows.Forms.CheckBox();
			this.jumpshot_button = new System.Windows.Forms.Button();
			this.log_checkBox = new System.Windows.Forms.CheckBox();
			((System.ComponentModel.ISupportInitialize)(this.nproc_numericUpDown)).BeginInit();
			this.SuspendLayout();
			// 
			// application_label
			// 
			this.application_label.Location = new System.Drawing.Point(24, 8);
			this.application_label.Name = "application_label";
			this.application_label.Size = new System.Drawing.Size(64, 23);
			this.application_label.TabIndex = 1;
			this.application_label.Text = "Application";
			// 
			// application_browse_button
			// 
			this.application_browse_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.application_browse_button.Location = new System.Drawing.Point(480, 8);
			this.application_browse_button.Name = "application_browse_button";
			this.application_browse_button.Size = new System.Drawing.Size(24, 21);
			this.application_browse_button.TabIndex = 3;
			this.application_browse_button.Text = "...";
			this.application_browse_button.Click += new System.EventHandler(this.application_browse_button_Click);
			// 
			// nproc_label
			// 
			this.nproc_label.Location = new System.Drawing.Point(24, 32);
			this.nproc_label.Name = "nproc_label";
			this.nproc_label.Size = new System.Drawing.Size(112, 23);
			this.nproc_label.TabIndex = 3;
			this.nproc_label.Text = "Number of processes";
			// 
			// nproc_numericUpDown
			// 
			this.nproc_numericUpDown.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.nproc_numericUpDown.Location = new System.Drawing.Point(456, 32);
			this.nproc_numericUpDown.Maximum = new System.Decimal(new int[] {
																				10000,
																				0,
																				0,
																				0});
			this.nproc_numericUpDown.Minimum = new System.Decimal(new int[] {
																				1,
																				0,
																				0,
																				0});
			this.nproc_numericUpDown.Name = "nproc_numericUpDown";
			this.nproc_numericUpDown.Size = new System.Drawing.Size(48, 20);
			this.nproc_numericUpDown.TabIndex = 4;
			this.nproc_numericUpDown.Value = new System.Decimal(new int[] {
																			  1,
																			  0,
																			  0,
																			  0});
			// 
			// wdir_label
			// 
			this.wdir_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.wdir_label.Location = new System.Drawing.Point(24, 368);
			this.wdir_label.Name = "wdir_label";
			this.wdir_label.Size = new System.Drawing.Size(96, 24);
			this.wdir_label.TabIndex = 5;
			this.wdir_label.Text = "working directory";
			// 
			// wdir_textBox
			// 
			this.wdir_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.wdir_textBox.Location = new System.Drawing.Point(152, 368);
			this.wdir_textBox.Name = "wdir_textBox";
			this.wdir_textBox.Size = new System.Drawing.Size(328, 20);
			this.wdir_textBox.TabIndex = 13;
			this.wdir_textBox.Text = "";
			// 
			// wdir_browse_button
			// 
			this.wdir_browse_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.wdir_browse_button.Location = new System.Drawing.Point(480, 368);
			this.wdir_browse_button.Name = "wdir_browse_button";
			this.wdir_browse_button.Size = new System.Drawing.Size(24, 20);
			this.wdir_browse_button.TabIndex = 14;
			this.wdir_browse_button.Text = "...";
			this.wdir_browse_button.Click += new System.EventHandler(this.wdir_browse_button_Click);
			// 
			// hosts_label
			// 
			this.hosts_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.hosts_label.Location = new System.Drawing.Point(24, 392);
			this.hosts_label.Name = "hosts_label";
			this.hosts_label.Size = new System.Drawing.Size(32, 23);
			this.hosts_label.TabIndex = 8;
			this.hosts_label.Text = "hosts";
			// 
			// hosts_textBox
			// 
			this.hosts_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.hosts_textBox.Location = new System.Drawing.Point(152, 392);
			this.hosts_textBox.Name = "hosts_textBox";
			this.hosts_textBox.Size = new System.Drawing.Size(312, 20);
			this.hosts_textBox.TabIndex = 15;
			this.hosts_textBox.Text = "";
			// 
			// hosts_reset_button
			// 
			this.hosts_reset_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.hosts_reset_button.Location = new System.Drawing.Point(464, 392);
			this.hosts_reset_button.Name = "hosts_reset_button";
			this.hosts_reset_button.Size = new System.Drawing.Size(40, 20);
			this.hosts_reset_button.TabIndex = 16;
			this.hosts_reset_button.Text = "reset";
			this.hosts_reset_button.Click += new System.EventHandler(this.hosts_reset_button_Click);
			// 
			// configfile_label
			// 
			this.configfile_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.configfile_label.Location = new System.Drawing.Point(24, 524);
			this.configfile_label.Name = "configfile_label";
			this.configfile_label.Size = new System.Drawing.Size(88, 16);
			this.configfile_label.TabIndex = 11;
			this.configfile_label.Text = "configuration file";
			// 
			// configfile_textBox
			// 
			this.configfile_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.configfile_textBox.Location = new System.Drawing.Point(152, 522);
			this.configfile_textBox.Name = "configfile_textBox";
			this.configfile_textBox.Size = new System.Drawing.Size(320, 20);
			this.configfile_textBox.TabIndex = 22;
			this.configfile_textBox.Text = "";
			// 
			// configfile_browse_button
			// 
			this.configfile_browse_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.configfile_browse_button.Location = new System.Drawing.Point(480, 522);
			this.configfile_browse_button.Name = "configfile_browse_button";
			this.configfile_browse_button.Size = new System.Drawing.Size(24, 20);
			this.configfile_browse_button.TabIndex = 23;
			this.configfile_browse_button.Text = "...";
			this.configfile_browse_button.Click += new System.EventHandler(this.configfile_browse_button_Click);
			// 
			// mpich1_label
			// 
			this.mpich1_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.mpich1_label.Location = new System.Drawing.Point(24, 548);
			this.mpich1_label.Name = "mpich1_label";
			this.mpich1_label.Size = new System.Drawing.Size(128, 16);
			this.mpich1_label.TabIndex = 14;
			this.mpich1_label.Text = "mpich1 configuration file";
			// 
			// mpich1_textBox
			// 
			this.mpich1_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.mpich1_textBox.Location = new System.Drawing.Point(152, 546);
			this.mpich1_textBox.Name = "mpich1_textBox";
			this.mpich1_textBox.Size = new System.Drawing.Size(320, 20);
			this.mpich1_textBox.TabIndex = 25;
			this.mpich1_textBox.Text = "";
			// 
			// mpich1_browse_button
			// 
			this.mpich1_browse_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.mpich1_browse_button.Location = new System.Drawing.Point(480, 546);
			this.mpich1_browse_button.Name = "mpich1_browse_button";
			this.mpich1_browse_button.Size = new System.Drawing.Size(24, 20);
			this.mpich1_browse_button.TabIndex = 26;
			this.mpich1_browse_button.Text = "...";
			this.mpich1_browse_button.Click += new System.EventHandler(this.mpich1_browse_button_Click);
			// 
			// env_label
			// 
			this.env_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.env_label.Location = new System.Drawing.Point(24, 416);
			this.env_label.Name = "env_label";
			this.env_label.Size = new System.Drawing.Size(120, 23);
			this.env_label.TabIndex = 17;
			this.env_label.Text = "environment variables";
			// 
			// env_textBox
			// 
			this.env_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.env_textBox.Location = new System.Drawing.Point(152, 416);
			this.env_textBox.Name = "env_textBox";
			this.env_textBox.Size = new System.Drawing.Size(352, 20);
			this.env_textBox.TabIndex = 17;
			this.env_textBox.Text = "";
			// 
			// drive_map_label
			// 
			this.drive_map_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.drive_map_label.Location = new System.Drawing.Point(24, 440);
			this.drive_map_label.Name = "drive_map_label";
			this.drive_map_label.Size = new System.Drawing.Size(88, 23);
			this.drive_map_label.TabIndex = 19;
			this.drive_map_label.Text = "drive mappings";
			// 
			// drive_map_textBox
			// 
			this.drive_map_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.drive_map_textBox.Location = new System.Drawing.Point(152, 440);
			this.drive_map_textBox.Name = "drive_map_textBox";
			this.drive_map_textBox.Size = new System.Drawing.Size(352, 20);
			this.drive_map_textBox.TabIndex = 18;
			this.drive_map_textBox.Text = "";
			// 
			// channel_label
			// 
			this.channel_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.channel_label.Location = new System.Drawing.Point(24, 464);
			this.channel_label.Name = "channel_label";
			this.channel_label.Size = new System.Drawing.Size(48, 23);
			this.channel_label.TabIndex = 21;
			this.channel_label.Text = "channel";
			// 
			// channel_comboBox
			// 
			this.channel_comboBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.channel_comboBox.Items.AddRange(new object[] {
																  "sock",
																  "nemesis",
																  "shm",
																  "ssm",
																  "mt",
																  "default",
																  "auto"});
			this.channel_comboBox.Location = new System.Drawing.Point(424, 464);
			this.channel_comboBox.Name = "channel_comboBox";
			this.channel_comboBox.Size = new System.Drawing.Size(80, 21);
			this.channel_comboBox.TabIndex = 19;
			this.channel_comboBox.Text = "default";
			// 
			// save_job_button
			// 
			this.save_job_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.save_job_button.Location = new System.Drawing.Point(429, 56);
			this.save_job_button.Name = "save_job_button";
			this.save_job_button.TabIndex = 8;
			this.save_job_button.Text = "Save Job";
			this.save_job_button.Click += new System.EventHandler(this.save_job_button_Click);
			// 
			// load_job_button
			// 
			this.load_job_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.load_job_button.Location = new System.Drawing.Point(352, 56);
			this.load_job_button.Name = "load_job_button";
			this.load_job_button.TabIndex = 7;
			this.load_job_button.Text = "Load Job";
			this.load_job_button.Click += new System.EventHandler(this.load_job_button_Click);
			// 
			// execute_button
			// 
			this.execute_button.Location = new System.Drawing.Point(8, 56);
			this.execute_button.Name = "execute_button";
			this.execute_button.TabIndex = 5;
			this.execute_button.Text = "Execute";
			this.execute_button.Click += new System.EventHandler(this.execute_button_Click);
			// 
			// break_button
			// 
			this.break_button.Enabled = false;
			this.break_button.Location = new System.Drawing.Point(88, 56);
			this.break_button.Name = "break_button";
			this.break_button.TabIndex = 6;
			this.break_button.Text = "Break";
			this.break_button.Click += new System.EventHandler(this.break_button_Click);
			// 
			// output_richTextBox
			// 
			this.output_richTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.output_richTextBox.Font = new System.Drawing.Font("Lucida Console", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.output_richTextBox.Location = new System.Drawing.Point(8, 104);
			this.output_richTextBox.Name = "output_richTextBox";
			this.output_richTextBox.Size = new System.Drawing.Size(496, 224);
			this.output_richTextBox.TabIndex = 11;
			this.output_richTextBox.Text = "";
			this.output_richTextBox.WordWrap = false;
			this.output_richTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.output_richTextBox_KeyPress);
			// 
			// application_comboBox
			// 
			this.application_comboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.application_comboBox.Location = new System.Drawing.Point(88, 8);
			this.application_comboBox.Name = "application_comboBox";
			this.application_comboBox.Size = new System.Drawing.Size(392, 21);
			this.application_comboBox.TabIndex = 2;
			// 
			// show_command_button
			// 
			this.show_command_button.Location = new System.Drawing.Point(8, 80);
			this.show_command_button.Name = "show_command_button";
			this.show_command_button.Size = new System.Drawing.Size(96, 23);
			this.show_command_button.TabIndex = 9;
			this.show_command_button.Text = "Show Command";
			this.show_command_button.Click += new System.EventHandler(this.show_command_button_Click);
			// 
			// command_line_textBox
			// 
			this.command_line_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.command_line_textBox.Location = new System.Drawing.Point(104, 80);
			this.command_line_textBox.Name = "command_line_textBox";
			this.command_line_textBox.ReadOnly = true;
			this.command_line_textBox.Size = new System.Drawing.Size(400, 20);
			this.command_line_textBox.TabIndex = 10;
			this.command_line_textBox.Text = "";
			// 
			// application_radioButton
			// 
			this.application_radioButton.Checked = true;
			this.application_radioButton.Location = new System.Drawing.Point(8, 8);
			this.application_radioButton.Name = "application_radioButton";
			this.application_radioButton.Size = new System.Drawing.Size(16, 24);
			this.application_radioButton.TabIndex = 1;
			this.application_radioButton.TabStop = true;
			this.application_radioButton.CheckedChanged += new System.EventHandler(this.application_radioButton_CheckedChanged);
			// 
			// configfile_radioButton
			// 
			this.configfile_radioButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.configfile_radioButton.Location = new System.Drawing.Point(8, 520);
			this.configfile_radioButton.Name = "configfile_radioButton";
			this.configfile_radioButton.Size = new System.Drawing.Size(16, 24);
			this.configfile_radioButton.TabIndex = 21;
			this.configfile_radioButton.CheckedChanged += new System.EventHandler(this.configfile_radioButton_CheckedChanged);
			// 
			// mpich1_radioButton
			// 
			this.mpich1_radioButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.mpich1_radioButton.Location = new System.Drawing.Point(8, 544);
			this.mpich1_radioButton.Name = "mpich1_radioButton";
			this.mpich1_radioButton.Size = new System.Drawing.Size(16, 24);
			this.mpich1_radioButton.TabIndex = 24;
			this.mpich1_radioButton.CheckedChanged += new System.EventHandler(this.mpich1_radioButton_CheckedChanged);
			// 
			// show_bottom_checkBox
			// 
			this.show_bottom_checkBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.show_bottom_checkBox.Location = new System.Drawing.Point(8, 328);
			this.show_bottom_checkBox.Name = "show_bottom_checkBox";
			this.show_bottom_checkBox.Size = new System.Drawing.Size(96, 16);
			this.show_bottom_checkBox.TabIndex = 12;
			this.show_bottom_checkBox.Text = "more options";
			this.show_bottom_checkBox.CheckedChanged += new System.EventHandler(this.show_bottom_checkBox_CheckedChanged);
			// 
			// extra_options_textBox
			// 
			this.extra_options_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.extra_options_textBox.Location = new System.Drawing.Point(152, 496);
			this.extra_options_textBox.Name = "extra_options_textBox";
			this.extra_options_textBox.Size = new System.Drawing.Size(352, 20);
			this.extra_options_textBox.TabIndex = 27;
			this.extra_options_textBox.Text = "";
			// 
			// extra_options_label
			// 
			this.extra_options_label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.extra_options_label.Location = new System.Drawing.Point(24, 496);
			this.extra_options_label.Name = "extra_options_label";
			this.extra_options_label.Size = new System.Drawing.Size(120, 23);
			this.extra_options_label.TabIndex = 28;
			this.extra_options_label.Text = "extra mpiexec options";
			// 
			// popup_checkBox
			// 
			this.popup_checkBox.Location = new System.Drawing.Point(168, 56);
			this.popup_checkBox.Name = "popup_checkBox";
			this.popup_checkBox.Size = new System.Drawing.Size(168, 24);
			this.popup_checkBox.TabIndex = 29;
			this.popup_checkBox.Text = "run in an separate window";
			// 
			// jumpshot_button
			// 
			this.jumpshot_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.jumpshot_button.Location = new System.Drawing.Point(432, 336);
			this.jumpshot_button.Name = "jumpshot_button";
			this.jumpshot_button.Size = new System.Drawing.Size(75, 24);
			this.jumpshot_button.TabIndex = 30;
			this.jumpshot_button.Text = "Jumpshot";
			this.jumpshot_button.Click += new System.EventHandler(this.jumpshot_button_Click);
			// 
			// log_checkBox
			// 
			this.log_checkBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.log_checkBox.Location = new System.Drawing.Point(312, 336);
			this.log_checkBox.Name = "log_checkBox";
			this.log_checkBox.Size = new System.Drawing.Size(112, 24);
			this.log_checkBox.TabIndex = 31;
			this.log_checkBox.Text = "produce clog2 file";
			// 
			// wmpiexec
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(512, 579);
			this.Controls.Add(this.log_checkBox);
			this.Controls.Add(this.jumpshot_button);
			this.Controls.Add(this.popup_checkBox);
			this.Controls.Add(this.extra_options_label);
			this.Controls.Add(this.extra_options_textBox);
			this.Controls.Add(this.application_radioButton);
			this.Controls.Add(this.command_line_textBox);
			this.Controls.Add(this.drive_map_textBox);
			this.Controls.Add(this.env_textBox);
			this.Controls.Add(this.mpich1_textBox);
			this.Controls.Add(this.configfile_textBox);
			this.Controls.Add(this.hosts_textBox);
			this.Controls.Add(this.wdir_textBox);
			this.Controls.Add(this.show_bottom_checkBox);
			this.Controls.Add(this.mpich1_radioButton);
			this.Controls.Add(this.configfile_radioButton);
			this.Controls.Add(this.show_command_button);
			this.Controls.Add(this.application_comboBox);
			this.Controls.Add(this.output_richTextBox);
			this.Controls.Add(this.break_button);
			this.Controls.Add(this.execute_button);
			this.Controls.Add(this.load_job_button);
			this.Controls.Add(this.save_job_button);
			this.Controls.Add(this.channel_comboBox);
			this.Controls.Add(this.channel_label);
			this.Controls.Add(this.drive_map_label);
			this.Controls.Add(this.env_label);
			this.Controls.Add(this.mpich1_browse_button);
			this.Controls.Add(this.mpich1_label);
			this.Controls.Add(this.configfile_browse_button);
			this.Controls.Add(this.configfile_label);
			this.Controls.Add(this.hosts_reset_button);
			this.Controls.Add(this.hosts_label);
			this.Controls.Add(this.wdir_browse_button);
			this.Controls.Add(this.wdir_label);
			this.Controls.Add(this.nproc_numericUpDown);
			this.Controls.Add(this.nproc_label);
			this.Controls.Add(this.application_browse_button);
			this.Controls.Add(this.application_label);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "wmpiexec";
			this.Text = "MPIEXEC wrapper";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.wmpiexec_Closing);
			((System.ComponentModel.ISupportInitialize)(this.nproc_numericUpDown)).EndInit();
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new wmpiexec());
		}

		private void AppendText(string str)
		{
			output_richTextBox.AppendText(str);
		}

		private void SetProcess(Process p)
		{
			process = p;
		}

		private void ResetExecuteButtons()
		{
			execute_button.Enabled = true;
			break_button.Enabled = false;
		}

		private void application_browse_button_Click(object sender, System.EventArgs e)
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = "Executable files (*.exe)|*.exe|All files (*.*)|*.*";
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				if (dlg.FileName.IndexOf(' ') != -1)
				{
					application_comboBox.Text = "\"" + dlg.FileName + "\"";
				}
				else
				{
					application_comboBox.Text = dlg.FileName;
				}
			}
		}

		private void wdir_browse_button_Click(object sender, System.EventArgs e)
		{
			FolderBrowserDialog dlg = new FolderBrowserDialog();
			if (wdir_textBox.Text.Length > 0)
			{
				dlg.SelectedPath = wdir_textBox.Text;
			}
			else
			{
				dlg.SelectedPath = Environment.CurrentDirectory;
			}
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				wdir_textBox.Text = dlg.SelectedPath;
			}
		}

		private void hosts_reset_button_Click(object sender, System.EventArgs e)
		{
			object obj;
			RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
			obj = key.GetValue("hosts");
			if (obj != null)
			{
				hosts_textBox.Text = obj.ToString();
			}
			else
			{
				hosts_textBox.Text = "";
			}
		}

		private void configfile_browse_button_Click(object sender, System.EventArgs e)
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = "All files (*.*)|*.*";
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				configfile_textBox.Text = dlg.FileName;
			}
		}

		private void mpich1_browse_button_Click(object sender, System.EventArgs e)
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = "Configuration file (*.cfg)|*.cfg|All files (*.*)|*.*";
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				mpich1_textBox.Text = dlg.FileName;
			}
		}

		private void save_job_button_Click(object sender, System.EventArgs e)
		{
			SaveFileDialog dlg = new SaveFileDialog();
			dlg.Filter = "Job files (*.txt *.mpi)|*.txt;*.mpi|All files (*.*)|*.*";
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				StreamWriter stream;
				using (stream = new StreamWriter(dlg.FileName))
				{
					stream.WriteLine("exe {0}", application_comboBox.Text.Trim());
					stream.WriteLine("n {0}", nproc_numericUpDown.Value);
					stream.WriteLine("wdir {0}", wdir_textBox.Text.Trim());
					stream.WriteLine("hosts {0}", hosts_textBox.Text.Trim());
					stream.WriteLine("cfg {0}", configfile_textBox.Text.Trim());
					stream.WriteLine("mpich1 {0}", mpich1_textBox.Text.Trim());
					stream.WriteLine("env {0}", env_textBox.Text.Trim());
					stream.WriteLine("map {0}", drive_map_textBox.Text.Trim());
					stream.WriteLine("channel {0}", channel_comboBox.Text.Trim());
					stream.WriteLine("extra {0}", extra_options_textBox.Text.Trim());
					stream.WriteLine(popup_checkBox.Checked ? "window yes" : "window no");
					stream.WriteLine(log_checkBox.Checked ? "log yes" : "log no");
					stream.Close();
				}
			}
		}

		private void load_job_button_Click(object sender, System.EventArgs e)
		{
			OpenFileDialog dlg = new OpenFileDialog();
			dlg.Filter = "Job files (*.txt *.mpi)|*.txt;*.mpi|All files (*.*)|*.*";
			if (dlg.ShowDialog() == DialogResult.OK)
			{
				StreamReader stream;
				using (stream = new StreamReader(dlg.FileName))
				{
					string line;
					int index;

					// reset all the values before loading new ones
					application_comboBox.Text = "";
					nproc_numericUpDown.Value = 1;
					wdir_textBox.Text = "";
					hosts_textBox.Text = "";
					configfile_textBox.Text = "";
					mpich1_textBox.Text = "";
					env_textBox.Text = "";
					drive_map_textBox.Text = "";
					channel_comboBox.Text = "";
					extra_options_textBox.Text = "";
					popup_checkBox.Checked = false;
					log_checkBox.Checked = false;

					line = stream.ReadLine();
					while (line != null)
					{
						index = line.IndexOf(" ");
						if (index > 0)
						{
							string key, val;
							key = line.Substring(0, index);
							val = line.Substring(index+1);
							switch (key)
							{
								case "exe":
									application_comboBox.Text = val;
									break;
								case "n":
									nproc_numericUpDown.Value = Convert.ToInt32(val);
									break;
								case "wdir":
									wdir_textBox.Text = val;
									break;
								case "hosts":
									hosts_textBox.Text = val;
									break;
								case "cfg":
									configfile_textBox.Text = val;
									break;
								case "mpich1":
									mpich1_textBox.Text = val;
									break;
								case "env":
									env_textBox.Text = val;
									break;
								case "map":
									drive_map_textBox.Text = val;
									break;
								case "channel":
									channel_comboBox.Text = val;
									break;
								case "extra":
									extra_options_textBox.Text = val;
									break;
								case "window":
									if (val == "yes")
										popup_checkBox.Checked = true;
									else
										popup_checkBox.Checked = false;
									break;
								case "log":
									if (val == "yes")
										log_checkBox.Checked = true;
									else
										log_checkBox.Checked = false;
									break;
							}
						}
						line = stream.ReadLine();
					}
					stream.Close();
				}
			}
		}

		#region Process thread functions
		/// <summary>
		/// These functions read and write input and output of the spawned mpiexec process.
		/// They are run in separate threads since they can execute for a long period of time
		/// and therefore shouldn't come from the thread pool (delegates).
		/// Is this a correct design decision?
		/// </summary>
		private void ReadOutput(TextReader stream)
		{
			thread_output_stream = stream;
			Thread thread = new Thread(new ThreadStart(ReadOutputThread));
			thread.Start();
		}

		private void ReadError(TextReader stream)
		{
			thread_error_stream = stream;
			Thread thread = new Thread(new ThreadStart(ReadErrorThread));
			thread.Start();
		}

		private void ReadErrorThread()
		{
			int num_read;
			char [] buffer = new char[4096];
			TextReader stream = thread_error_stream;
			num_read = stream.Read(buffer, 0, 4096);
			while (num_read > 0)
			{
				string str;
				char [] text = new char[num_read];
				Array.Copy(buffer, 0, text, 0, num_read);
				str = new string(text);
				object[] pList = { str };
				// put the string in the edit box in a thread safe way
				AppendTextDelegate ap = new AppendTextDelegate(AppendText);
				output_richTextBox.Invoke(ap, pList);
				num_read = stream.Read(buffer, 0, 1024);
			}
		}

		private void ReadOutputThread()
		{
			int num_read;
			char [] buffer = new char[4096];
			TextReader stream = thread_output_stream;
			num_read = stream.Read(buffer, 0, 4096);
			while (num_read > 0)
			{
				string str;
				char [] text = new char[num_read];
				Array.Copy(buffer, 0, text, 0, num_read);
				str = new string(text);
				object[] pList = { str };
				// put the string in the edit box in a thread safe way
				AppendTextDelegate ap = new AppendTextDelegate(AppendText);
				output_richTextBox.Invoke(ap, pList);
				num_read = stream.Read(buffer, 0, 1024);
			}
		}

		ManualResetEvent char_available = new ManualResetEvent(false);
		ManualResetEvent char_written = new ManualResetEvent(true);
		bool quit_input = false;
		char ch;
		private void WriteInput(TextWriter stream)
		{
			thread_input_stream = stream;
			Thread thread = new Thread(new ThreadStart(WriteInputThread));
			thread.Start();
		}

		private void WriteInputThread()
		{
			WriteInputEx(thread_input_stream);
		}

		private void WriteInputEx(TextWriter stream)
		{
			while (char_available.WaitOne())
			{
				if (quit_input)
				{
					return;
				}
				stream.Write(ch);
				stream.Flush();
				char_available.Reset();
				char_written.Set();

				/*
				object[] pList = { "ch(" + ch + ")" };
				// put the string in the edit box in a thread safe way
				AppendTextDelegate ap = new AppendTextDelegate(AppendText);
				output_richTextBox.Invoke(ap, pList);
				*/
			}
		}

		private void RunCommand(string command, string mpiexec_command, string mpiexec_command_args, bool popup)
		{
			thread_command = command;
			thread_mpiexec_command = mpiexec_command;
			thread_mpiexec_command_args = mpiexec_command_args;
			thread_popup = popup;
			Thread thread = new Thread(new ThreadStart(RunCommandThread));
			thread.Start();
		}

		void RunCommandThread()
		{
			RunCommandEx(thread_command, thread_mpiexec_command, thread_mpiexec_command_args, thread_popup);
		}

		private void RunCommandEx(string command, string mpiexec_command, string mpiexec_command_args, bool popup)
		{
			if (popup)
			{
				// start a process to run the mpiexec command
				Process p = new Process();
				p.StartInfo.UseShellExecute = true;
				p.StartInfo.CreateNoWindow = false;
				p.StartInfo.RedirectStandardError = false;
				p.StartInfo.RedirectStandardInput = false;
				p.StartInfo.RedirectStandardOutput = false;
				p.StartInfo.FileName = "cmd.exe";
				if (command.IndexOf('"') != -1)
				{
					p.StartInfo.Arguments = "/C \"" + command + "\" && pause";
				}
				else
				{
					p.StartInfo.Arguments = "/C " + command + " && pause";
				}
				p.Start();
				p.WaitForExit();
			}
			else
			{
				quit_input = false;
				// start a process to run the mpiexec command
				Process p = new Process();
				p.StartInfo.UseShellExecute = false;
				p.StartInfo.CreateNoWindow = true;
				p.StartInfo.RedirectStandardError = true;
				p.StartInfo.RedirectStandardInput = true;
				p.StartInfo.RedirectStandardOutput = true;
				p.StartInfo.FileName = mpiexec_command;
				p.StartInfo.Arguments = mpiexec_command_args;
				p.Start();
				// start a delagate to read stdout
				ReadOutputDelegate r1 = new ReadOutputDelegate(ReadOutput);
				//IAsyncResult ar1 = r1.BeginInvoke(TextReader.Synchronized(p.StandardOutput), null, null);
				object [] list1 = { TextReader.Synchronized(p.StandardOutput) };
				Invoke(r1, list1);
				// start a delagate to read stderr
				ReadErrorDelegate r2 = new ReadErrorDelegate(ReadError);
				//IAsyncResult ar2 = r2.BeginInvoke(TextReader.Synchronized(p.StandardError), null, null);
				object [] list2 = { TextReader.Synchronized(p.StandardError) };
				Invoke(r2, list2);
				// Send stdin to the process
				WriteInputDelegate w = new WriteInputDelegate(WriteInput);
				//IAsyncResult ar3 = w.BeginInvoke(TextWriter.Synchronized(p.StandardInput), null, null);
				object [] list3 = { TextWriter.Synchronized(p.StandardInput) };
				Invoke(w, list3);

				//process = p;
				SetProcessDelegate sp = new SetProcessDelegate(SetProcess);
				object [] list = { p };
				Invoke(sp, list);
				// wait for the process to exit
				p.WaitForExit();
				// wait for the output and error delegates to finish
				//r1.EndInvoke(ar1);
				//r2.EndInvoke(ar2);
				// set the process variable to null
				list[0] = null;
				Invoke(sp, list);
				// signal the input delegate to exit after setting the process variable to null to avoid conflicts with the keypressed handler
				quit_input = true;
				char_available.Set();
				//w.EndInvoke(ar3);
			}
			ResetExecuteButtonsDelegate d = new ResetExecuteButtonsDelegate(ResetExecuteButtons);
			Invoke(d);
		}
		#endregion

		private void execute_button_Click(object sender, System.EventArgs e)
		{
			VerifyEncryptedPasswordExists();
			execute_button.Enabled = false;
			break_button.Enabled = true;
			output_richTextBox.Clear();
			if (last_execute_result != null)
			{
				run_command.EndInvoke(last_execute_result);
			}
			// Create the command line
			command_line_textBox.Text = get_command_line();
			// Add the command to the application drop down if it has not already been added
			bool found = false;
			foreach (string str in application_comboBox.Items)
			{
				if (str == application_comboBox.Text)
					found = true;
			}
			if (!found)
			{
				application_comboBox.Items.Add(application_comboBox.Text);
			}
			run_command = new RunCommandDelegate(RunCommand);
			last_execute_result = run_command.BeginInvoke(command_line_textBox.Text, mpiexec_command, mpiexec_command_args, popup_checkBox.Checked, null, null);
			// after starting a command move to the output box so the user can interact with the running process.
			output_richTextBox.Focus();
		}

		private void VerifyEncryptedPasswordExists()
		{
			try
			{
				bool popup = true;
				string mpiexec = get_mpiexec();

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

				// Or run "mpiexec -validate" and check the output for SUCCESS
				// This code will last longer because it doesn't rely on known information about the smpd implementation
				// The disadvantage of this code is that the user credentials have to be valid on the local host.
				/*
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
				*/

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

		private void break_button_Click(object sender, System.EventArgs e)
		{
			if (process != null)
			{
				process.Kill();
			}
		}

		#region Get known files: mpiexec, jumpshot, and clog2 files
		private string get_jumpshot()
		{
			string jumpshot = "";
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
						jumpshot = obj.ToString();
						if (jumpshot.EndsWith(@"\"))
						{
							jumpshot = jumpshot + @"bin\jumpshot.jar";
						}
						else
						{
							jumpshot = jumpshot + @"\bin\jumpshot.jar";
						}
						if (!File.Exists(jumpshot))
						{
							jumpshot = "";
						}
					}
				}
				if (jumpshot == "")
				{
					key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
					if (key != null)
					{
						obj = key.GetValue("binary");
						key.Close();
						if (obj != null)
						{
							jumpshot = obj.ToString().Replace("smpd.exe", "jumpshot.jar");
							if (!File.Exists(jumpshot))
							{
								jumpshot = "";
							}
						}
					}
				}
				if (jumpshot == "")
				{
					jumpshot = "jumpshot.jar";
				}
				jumpshot = jumpshot.Trim();
				if (jumpshot.IndexOf(' ') != -1)
				{
					jumpshot = "\"" + jumpshot + "\"";
				}
			}
			catch (Exception)
			{
				jumpshot = "jumpshot.jar";
			}
			return jumpshot;
		}

		private string get_clog2()
		{
			int index, last;
			string app = application_comboBox.Text;
			try
			{
				if (app[0] == '\"')
				{
					// remove the quotes to be restored at the end
					app = app.Replace("\"", "");
				}
				index = app.LastIndexOf('\\');
				if (index == -1)
				{
					index = 0;
				}
				last = index;
				// find the index of .exe starting from the last \
				index = app.IndexOf(".exe", last);
				if (index == -1)
				{
					// no .exe so find the first space after the last \
					// This assumes the executable does not have spaces in its name
					index = app.IndexOf(' ', last);
					if (index == -1)
					{
						// no .exe and no spaces found so take the entire string
						index = app.Length;
					}
				}
				app = app.Substring(0, index) + ".exe.clog2";
				app = app.Trim();
				if (!File.Exists(app) && wdir_textBox.Text.Length > 0)
				{
					// file not found from the application combo box so try finding it in the working directory
					string app2 = application_comboBox.Text;
					if (app2[0] == '\"')
					{
						// remove the quotes to be restored at the end
						app2 = app2.Replace("\"", "");
					}
					index = app2.LastIndexOf('\\');
					if (index != -1)
					{
						// remove the path
						app2 = app2.Remove(0, index);
					}
					// find the index of .exe
					index = app2.IndexOf(".exe");
					if (index != -1)
					{
						// remove any arguments after the executable name
						app2 = app2.Substring(0, index + 4);
					}
					else
					{
						// no .exe so find the first space
						// This assumes the executable does not have spaces in its name
						index = app.IndexOf(' ');
						if (index != -1)
						{
							app2 = app2.Substring(0, index) + ".exe";
						}
						else
						{
							// no .exe and no arguments so append .exe
							app2 += ".exe";
						}
					}
					// app2 should now contain only the executable name
					// Add the wdir path and clog2 extension
					if (wdir_textBox.Text.EndsWith("\\"))
						app2 = wdir_textBox.Text + app2 + ".clog2";
					else
						app2 = wdir_textBox.Text + "\\" + app2 + ".clog2";
					if (File.Exists(app2))
					{
						app = app2;
						FileStream f = File.OpenRead(app2);
						app = f.Name;
						f.Close();
					}
				}
				if (app.IndexOf(' ') != -1)
				{
					app = "\"" + app + "\"";
				}
			}
			catch (Exception)
			{
				app = "";
			}
			return app;
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
				mpiexec_command = mpiexec; // save the path to mpiexec.exe without quotes
				if (mpiexec.IndexOf(' ') != -1)
				{
					mpiexec = "\"" + mpiexec + "\"";
				}
			}
			catch (Exception)
			{
				mpiexec = "mpiexec.exe";
			}
			return mpiexec;
		}
		#endregion

		private string get_command_line()
		{
			string cmd, mpiexec;
			cmd = mpiexec = get_mpiexec();
			if (application_radioButton.Checked)
			{
				wdir_textBox.Text = wdir_textBox.Text.Trim();
				if (wdir_textBox.Text.Length > 0)
				{
					if (wdir_textBox.Text.IndexOf(' ') != -1)
					{
						cmd = cmd + " -wdir \"" + wdir_textBox.Text + "\"";
					}
					else
					{
						cmd = cmd + " -wdir " + wdir_textBox.Text;
					}
				}
				drive_map_textBox.Text = drive_map_textBox.Text.Trim();
				if (drive_map_textBox.Text.Length > 0)
				{
					if (drive_map_textBox.Text.IndexOf(' ') != -1)
					{
						cmd = cmd + " -map \"" + drive_map_textBox.Text + "\"";
					}
					else
					{
						cmd = cmd + " -map " + drive_map_textBox.Text;
					}
				}
				if (log_checkBox.Checked)
				{
					cmd = cmd + " -log";
				}
				channel_comboBox.Text = channel_comboBox.Text.Trim();
				if (channel_comboBox.Text != "default" && channel_comboBox.Text != "auto" && channel_comboBox.Text != "")
				{
					cmd = cmd + " -env MPICH2_CHANNEL " + channel_comboBox.Text;
				}
				env_textBox.Text = env_textBox.Text.Trim();
				if (env_textBox.Text.Length > 0)
				{
					foreach (string keyval in env_textBox.Text.Split(' '))
					{
						string [] kv;
						kv = keyval.Split('=');
						if (kv.Length > 1)
						{
							cmd = cmd + " -env " + kv[0] + " " + kv[1];
						}
					}
				}
				hosts_textBox.Text = hosts_textBox.Text.Trim();
				if (hosts_textBox.Text.Length > 0)
				{
					string [] hosts;
					int n;
					hosts = hosts_textBox.Text.Split(' ');
					n = (int)nproc_numericUpDown.Value;
					if (n > hosts.Length)
					{
						int max = n % hosts.Length;
						cmd = cmd + " -hosts " + hosts.Length + " ";
						for (int i=0; i<hosts.Length; i++)
						{
							if (i<max)
							{
								cmd = cmd + hosts[i] + " " + ((n / hosts.Length) + 1) + " ";
							}
							else
							{
								cmd = cmd + hosts[i] + " " + (n / hosts.Length) + " ";
							}
						}
					}
					else
					{
						cmd = cmd + " -hosts " + n + " ";
						for (int i=0; i<n; i++)
						{
							cmd = cmd + hosts[i] + " ";
						}
					}
				}
				else
				{
					cmd = cmd + " -n " + nproc_numericUpDown.Value + " ";
				}
				extra_options_textBox.Text = extra_options_textBox.Text.Trim();
				if (extra_options_textBox.Text.Length > 0)
				{
					cmd = cmd + " " + extra_options_textBox.Text;
				}
				application_comboBox.Text = application_comboBox.Text.Trim();
				// Don't add quotes because they would enclose any arguments to the application
				// The user must provide the quotes as necessary in the combo box.
				/*
				if (application_comboBox.Text.IndexOf(' ') != -1)
				{
					cmd = cmd + " \"" + application_comboBox.Text + "\"";
				}
				else
				{
					cmd = cmd + " " + application_comboBox.Text;
				}
				*/
				cmd = cmd + " -noprompt " + application_comboBox.Text; // add -noprompt since we can't send input to stdin
				mpiexec_command_args = cmd.Replace(mpiexec, ""); // save the command line minus the mpiexec path
			}
			else if (configfile_radioButton.Checked)
			{
				configfile_textBox.Text = configfile_textBox.Text.Trim();
				if (configfile_textBox.Text.IndexOf(' ') != -1)
				{
					cmd = cmd + " -configfile \"" + configfile_textBox.Text + "\"";
				}
				else
				{
					cmd = cmd + " -configfile " + configfile_textBox.Text;
				}
			}
			else if (mpich1_radioButton.Checked)
			{
				mpich1_textBox.Text = mpich1_textBox.Text.Trim();
				if (mpich1_textBox.Text.IndexOf(' ') != -1)
				{
					cmd = cmd + " -file \"" + mpich1_textBox.Text + "\"";
				}
				else
				{
					cmd = cmd + " -file " + mpich1_textBox.Text;
				}
			}
			else
			{
				cmd = "";
			}
			return cmd;
		}

		private void show_command_button_Click(object sender, System.EventArgs e)
		{
			command_line_textBox.Text = get_command_line();
		}

		private void output_richTextBox_KeyPress(object sender, System.Windows.Forms.KeyPressEventArgs e)
		{
			if (process != null)
			{
				char_written.WaitOne();
				ch = e.KeyChar;
				char_written.Reset();
				char_available.Set();
			}
		}

		#region Radio buttons enabling and disabling functions
		/// <summary>
		/// Enable and disable controls based on which radio button is selected
		/// </summary>
		private void DisableApplicationControls()
		{
			application_label.Enabled = false;
			application_comboBox.Enabled = false;
			application_browse_button.Enabled = false;
			nproc_label.Enabled = false;
			nproc_numericUpDown.Enabled = false;
			wdir_label.Enabled = false;
			wdir_textBox.Enabled = false;
			wdir_browse_button.Enabled = false;
			hosts_label.Enabled = false;
			hosts_textBox.Enabled = false;
			hosts_reset_button.Enabled = false;
			env_label.Enabled = false;
			env_textBox.Enabled = false;
			drive_map_label.Enabled = false;
			drive_map_textBox.Enabled = false;
			channel_label.Enabled = false;
			channel_comboBox.Enabled = false;
			extra_options_label.Enabled = false;
			extra_options_textBox.Enabled = false;
			log_checkBox.Enabled = false;
		}

		private void EnableApplicationControls()
		{
			application_label.Enabled = true;
			application_comboBox.Enabled = true;
			application_browse_button.Enabled = true;
			nproc_label.Enabled = true;
			nproc_numericUpDown.Enabled = true;
			wdir_label.Enabled = true;
			wdir_textBox.Enabled = true;
			wdir_browse_button.Enabled = true;
			hosts_label.Enabled = true;
			hosts_textBox.Enabled = true;
			hosts_reset_button.Enabled = true;
			env_label.Enabled = true;
			env_textBox.Enabled = true;
			drive_map_label.Enabled = true;
			drive_map_textBox.Enabled = true;
			channel_label.Enabled = true;
			channel_comboBox.Enabled = true;
			extra_options_textBox.Enabled = true;
			extra_options_label.Enabled = true;
			log_checkBox.Enabled = true;
		}

		private void DisableConfigfileControls()
		{
			configfile_label.Enabled = false;
			configfile_textBox.Enabled = false;
			configfile_browse_button.Enabled = false;
		}

		private void EnableConfigfileControls()
		{
			configfile_label.Enabled = true;
			configfile_textBox.Enabled = true;
			configfile_browse_button.Enabled = true;
		}

		private void DisableMPICH1Controls()
		{
			mpich1_label.Enabled = false;
			mpich1_textBox.Enabled = false;
			mpich1_browse_button.Enabled = false;
		}

		private void EnableMPICH1Controls()
		{
			mpich1_label.Enabled = true;
			mpich1_textBox.Enabled = true;
			mpich1_browse_button.Enabled = true;
		}

		private void application_radioButton_CheckedChanged(object sender, System.EventArgs e)
		{
			if (application_radioButton.Checked)
			{
				EnableApplicationControls();
				DisableConfigfileControls();
				DisableMPICH1Controls();
			}
		}

		private void configfile_radioButton_CheckedChanged(object sender, System.EventArgs e)
		{
			if (configfile_radioButton.Checked)
			{
				DisableApplicationControls();
				EnableConfigfileControls();
				DisableMPICH1Controls();
			}
		}

		private void mpich1_radioButton_CheckedChanged(object sender, System.EventArgs e)
		{
			if (mpich1_radioButton.Checked)
			{
				DisableApplicationControls();
				DisableConfigfileControls();
				EnableMPICH1Controls();
			}
		}
		#endregion

		#region Dialog expansion and contraction
		/// <summary>
		/// Manage expansion and contraction of the extra dialog controls
		/// </summary>
		private void DisableAnchors()
		{
			show_bottom_checkBox.Anchor = AnchorStyles.Top;
			wdir_label.Anchor = AnchorStyles.Top;
			wdir_textBox.Anchor = AnchorStyles.Top;
			wdir_browse_button.Anchor = AnchorStyles.Top;
			hosts_label.Anchor = AnchorStyles.Top;
			hosts_textBox.Anchor = AnchorStyles.Top;
			hosts_reset_button.Anchor = AnchorStyles.Top;
			configfile_label.Anchor = AnchorStyles.Top;
			configfile_textBox.Anchor = AnchorStyles.Top;
			configfile_browse_button.Anchor = AnchorStyles.Top;
			mpich1_label.Anchor = AnchorStyles.Top;
			mpich1_textBox.Anchor = AnchorStyles.Top;
			mpich1_browse_button.Anchor = AnchorStyles.Top;
			env_label.Anchor = AnchorStyles.Top;
			env_textBox.Anchor = AnchorStyles.Top;
			drive_map_label.Anchor = AnchorStyles.Top;
			drive_map_textBox.Anchor = AnchorStyles.Top;
			channel_label.Anchor = AnchorStyles.Top;
			channel_comboBox.Anchor = AnchorStyles.Top;
			output_richTextBox.Anchor = AnchorStyles.Top;
			configfile_radioButton.Anchor = AnchorStyles.Top;
			mpich1_radioButton.Anchor = AnchorStyles.Top;
			extra_options_label.Anchor = AnchorStyles.Top;
			extra_options_textBox.Anchor = AnchorStyles.Top;
			log_checkBox.Anchor = AnchorStyles.Top;
			jumpshot_button.Anchor = AnchorStyles.Top;
		}

		private void EnableAnchors()
		{
			show_bottom_checkBox.Anchor = (AnchorStyles.Bottom | AnchorStyles.Left);
			wdir_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			wdir_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			wdir_browse_button.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			hosts_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			hosts_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			hosts_reset_button.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			configfile_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			configfile_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			configfile_browse_button.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			mpich1_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			mpich1_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			mpich1_browse_button.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			env_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			env_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			drive_map_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			drive_map_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			channel_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			channel_comboBox.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			output_richTextBox.Anchor = ((AnchorStyles)((((AnchorStyles.Top | AnchorStyles.Bottom) | AnchorStyles.Left) | AnchorStyles.Right)));
			configfile_radioButton.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			mpich1_radioButton.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			extra_options_label.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Left)));
			extra_options_textBox.Anchor = ((AnchorStyles)(((AnchorStyles.Bottom | AnchorStyles.Left) | AnchorStyles.Right)));
			log_checkBox.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
			jumpshot_button.Anchor = ((AnchorStyles)((AnchorStyles.Bottom | AnchorStyles.Right)));
		}

		private void HideExtraOptions()
		{
			wdir_label.Hide();
			wdir_textBox.Hide();
			wdir_browse_button.Hide();
			hosts_label.Hide();
			hosts_textBox.Hide();
			hosts_reset_button.Hide();
			configfile_label.Hide();
			configfile_textBox.Hide();
			configfile_browse_button.Hide();
			mpich1_label.Hide();
			mpich1_textBox.Hide();
			mpich1_browse_button.Hide();
			env_label.Hide();
			env_textBox.Hide();
			drive_map_label.Hide();
			drive_map_textBox.Hide();
			channel_label.Hide();
			channel_comboBox.Hide();
			configfile_radioButton.Hide();
			mpich1_radioButton.Hide();
			extra_options_textBox.Hide();
			extra_options_label.Hide();
			log_checkBox.Hide();
			jumpshot_button.Hide();
		}

		private void ShowExtraOptions()
		{
			wdir_label.Show();
			wdir_textBox.Show();
			wdir_browse_button.Show();
			hosts_label.Show();
			hosts_textBox.Show();
			hosts_reset_button.Show();
			configfile_label.Show();
			configfile_textBox.Show();
			configfile_browse_button.Show();
			mpich1_label.Show();
			mpich1_textBox.Show();
			mpich1_browse_button.Show();
			env_label.Show();
			env_textBox.Show();
			drive_map_label.Show();
			drive_map_textBox.Show();
			channel_label.Show();
			channel_comboBox.Show();
			configfile_radioButton.Show();
			mpich1_radioButton.Show();
			extra_options_label.Show();
			extra_options_textBox.Show();
			log_checkBox.Show();
			jumpshot_button.Show();
		}

		private void UpdateExtraControls(bool bShow)
		{
			DisableAnchors();
			if (bShow)
			{
				ShowExtraOptions();
				Height = Height + expanded_dialog_difference;//230;
			}
			else
			{
				HideExtraOptions();
				Height = Height - expanded_dialog_difference;//230;
			}
			EnableAnchors();
		}

		private void show_bottom_checkBox_CheckedChanged(object sender, System.EventArgs e)
		{
			UpdateExtraControls(show_bottom_checkBox.Checked);
		}
		#endregion

		private void wmpiexec_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{
			if (process != null)
			{
				process.Kill();
			}
			if (last_execute_result != null)
			{
				run_command.EndInvoke(last_execute_result);
				last_execute_result = null;
			}
		}

		private void jumpshot_button_Click(object sender, System.EventArgs e)
		{
			Process p;
			string args = "-Xms32m -Xmx256m -jar " + get_jumpshot() + " " + get_clog2();
			//MessageBox.Show("javaw.exe " + args);
			//output_richTextBox.AppendText("javaw.exe " + args + "\r\n");
			try
			{
				p = Process.Start("javaw.exe", args);
			}
			catch (Exception x)
			{
				MessageBox.Show("Unable to start: javaw.exe " + args + "\r\n" + x.Message, "Error");
			}
		}
	}
}
