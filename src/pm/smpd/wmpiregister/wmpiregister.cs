using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Diagnostics;
using Microsoft.Win32;
using System.IO;

namespace wmpiregister
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class wmpiregister : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label account_label;
		private System.Windows.Forms.TextBox account_textBox;
		private System.Windows.Forms.Label password_label;
		private System.Windows.Forms.TextBox password_textBox;
		private System.Windows.Forms.Button register_button;
		private System.Windows.Forms.Button remove_button;
		private System.Windows.Forms.Button cancel_button;
		private System.Windows.Forms.Label usage_label;
		private System.Windows.Forms.Label usage2_label;
		private System.Windows.Forms.Label example_label;
		private System.Windows.Forms.TextBox result_textBox;
		private System.Windows.Forms.Button ok_button;
		private Label usage3_label;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public wmpiregister()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
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
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(wmpiregister));
			this.account_label = new System.Windows.Forms.Label();
			this.account_textBox = new System.Windows.Forms.TextBox();
			this.password_label = new System.Windows.Forms.Label();
			this.password_textBox = new System.Windows.Forms.TextBox();
			this.register_button = new System.Windows.Forms.Button();
			this.remove_button = new System.Windows.Forms.Button();
			this.cancel_button = new System.Windows.Forms.Button();
			this.usage_label = new System.Windows.Forms.Label();
			this.usage2_label = new System.Windows.Forms.Label();
			this.example_label = new System.Windows.Forms.Label();
			this.result_textBox = new System.Windows.Forms.TextBox();
			this.ok_button = new System.Windows.Forms.Button();
			this.usage3_label = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// account_label
			// 
			this.account_label.Location = new System.Drawing.Point(24, 152);
			this.account_label.Name = "account_label";
			this.account_label.TabIndex = 0;
			this.account_label.Text = "Account:";
			// 
			// account_textBox
			// 
			this.account_textBox.Location = new System.Drawing.Point(128, 152);
			this.account_textBox.Name = "account_textBox";
			this.account_textBox.TabIndex = 1;
			// 
			// password_label
			// 
			this.password_label.Location = new System.Drawing.Point(24, 184);
			this.password_label.Name = "password_label";
			this.password_label.TabIndex = 2;
			this.password_label.Text = "password";
			// 
			// password_textBox
			// 
			this.password_textBox.Location = new System.Drawing.Point(128, 184);
			this.password_textBox.Name = "password_textBox";
			this.password_textBox.PasswordChar = '*';
			this.password_textBox.TabIndex = 3;
			// 
			// register_button
			// 
			this.register_button.Location = new System.Drawing.Point(8, 253);
			this.register_button.Name = "register_button";
			this.register_button.Size = new System.Drawing.Size(64, 23);
			this.register_button.TabIndex = 4;
			this.register_button.Text = "Register";
			this.register_button.Click += new System.EventHandler(this.register_button_Click);
			// 
			// remove_button
			// 
			this.remove_button.Location = new System.Drawing.Point(80, 253);
			this.remove_button.Name = "remove_button";
			this.remove_button.Size = new System.Drawing.Size(64, 23);
			this.remove_button.TabIndex = 5;
			this.remove_button.Text = "Remove";
			this.remove_button.Click += new System.EventHandler(this.remove_button_Click);
			// 
			// cancel_button
			// 
			this.cancel_button.Location = new System.Drawing.Point(216, 253);
			this.cancel_button.Name = "cancel_button";
			this.cancel_button.Size = new System.Drawing.Size(64, 23);
			this.cancel_button.TabIndex = 7;
			this.cancel_button.Text = "Cancel";
			this.cancel_button.Click += new System.EventHandler(this.cancel_button_Click);
			// 
			// usage_label
			// 
			this.usage_label.Location = new System.Drawing.Point(24, 8);
			this.usage_label.Name = "usage_label";
			this.usage_label.Size = new System.Drawing.Size(224, 48);
			this.usage_label.TabIndex = 9;
			this.usage_label.Text = "Use this tool to encrypt an account and password to be used by mpiexec to launch " +
				"mpich2 jobs.";
			// 
			// usage2_label
			// 
			this.usage2_label.Location = new System.Drawing.Point(24, 56);
			this.usage2_label.Name = "usage2_label";
			this.usage2_label.Size = new System.Drawing.Size(224, 48);
			this.usage2_label.TabIndex = 10;
			this.usage2_label.Text = "The account provided must be a valid user account available on all the nodes that" +
				" will participate in mpich2 jobs.";
			// 
			// example_label
			// 
			this.example_label.Location = new System.Drawing.Point(24, 104);
			this.example_label.Name = "example_label";
			this.example_label.Size = new System.Drawing.Size(120, 40);
			this.example_label.TabIndex = 11;
			this.example_label.Text = "Example: mydomain\\myaccount or myaccount";
			// 
			// result_textBox
			// 
			this.result_textBox.Location = new System.Drawing.Point(8, 285);
			this.result_textBox.Multiline = true;
			this.result_textBox.Name = "result_textBox";
			this.result_textBox.ReadOnly = true;
			this.result_textBox.Size = new System.Drawing.Size(272, 64);
			this.result_textBox.TabIndex = 8;
			// 
			// ok_button
			// 
			this.ok_button.Location = new System.Drawing.Point(152, 253);
			this.ok_button.Name = "ok_button";
			this.ok_button.Size = new System.Drawing.Size(56, 23);
			this.ok_button.TabIndex = 6;
			this.ok_button.Text = "OK";
			this.ok_button.Click += new System.EventHandler(this.ok_button_Click);
			// 
			// usage3_label
			// 
			this.usage3_label.Location = new System.Drawing.Point(24, 210);
			this.usage3_label.Name = "usage3_label";
			this.usage3_label.Size = new System.Drawing.Size(224, 37);
			this.usage3_label.TabIndex = 12;
			this.usage3_label.Text = "Click register to save the credentials and Remove to delete the credentials for t" +
				"he current user.";
			// 
			// wmpiregister
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(288, 359);
			this.Controls.Add(this.usage3_label);
			this.Controls.Add(this.ok_button);
			this.Controls.Add(this.result_textBox);
			this.Controls.Add(this.example_label);
			this.Controls.Add(this.usage2_label);
			this.Controls.Add(this.usage_label);
			this.Controls.Add(this.cancel_button);
			this.Controls.Add(this.remove_button);
			this.Controls.Add(this.register_button);
			this.Controls.Add(this.password_textBox);
			this.Controls.Add(this.password_label);
			this.Controls.Add(this.account_textBox);
			this.Controls.Add(this.account_label);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "wmpiregister";
			this.Text = "MPIEXEC -register wrapper";
			this.ResumeLayout(false);
			this.PerformLayout();

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new wmpiregister());
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

		private void register_button_Click(object sender, System.EventArgs e)
		{
			string output;
			//string error;
			try
			{
				if (password_textBox.Text.Length == 0)
				{
					MessageBox.Show("MPICH2 cannot use user credentials with empty passwords, please select another user");
					return;
				}
				Process process = new Process();
				process.StartInfo.FileName = get_mpiexec();
				process.StartInfo.Arguments = "-register";
				process.StartInfo.RedirectStandardInput = true;
				process.StartInfo.RedirectStandardOutput = true;
				//process.StartInfo.RedirectStandardError = true;
				process.StartInfo.RedirectStandardError = false;
				process.StartInfo.UseShellExecute = false;
				process.StartInfo.CreateNoWindow = true;
				process.Start();
				process.StandardInput.WriteLine(account_textBox.Text);
				process.StandardInput.WriteLine(password_textBox.Text);
				process.StandardInput.WriteLine(password_textBox.Text);
				//error = process.StandardError.ReadToEnd();
				output = process.StandardOutput.ReadToEnd();
				process.WaitForExit();
				result_textBox.Text = output; // + "\n" + error;
			}
			catch (Exception x)
			{
				result_textBox.Text = "Unable to run \"mpiexec -register\"\r\nError: " + x.Message;
			}
		}

		private void remove_button_Click(object sender, System.EventArgs e)
		{
			string output;
			//string error;
			try
			{
				Process process = new Process();
				process.StartInfo.FileName = get_mpiexec();
				process.StartInfo.Arguments = "-remove";
				process.StartInfo.RedirectStandardInput = false;
				process.StartInfo.RedirectStandardOutput = true;
				//process.StartInfo.RedirectStandardError = true;
				process.StartInfo.RedirectStandardError = false;
				process.StartInfo.UseShellExecute = false;
				process.StartInfo.CreateNoWindow = true;
				process.Start();
				//error = process.StandardError.ReadToEnd();
				output = process.StandardOutput.ReadToEnd();
				process.WaitForExit();
				result_textBox.Text = output;// + "\n" + error;
			}
			catch (Exception x)
			{
				result_textBox.Text = "Unable to run \"mpiexec -remove\"\r\nError: " + x.Message;
			}
		}

		private void cancel_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void ok_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}
	}
}
