/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;
using System.Text;
using System.Collections.Specialized;

namespace MandelViewer
{
	public class MandelViewerForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button connect_button;
		private System.Windows.Forms.TextBox host;
		private System.Windows.Forms.TextBox port;
		private System.Windows.Forms.PictureBox outputBox;
		private bool bConnected = false;
		private TcpClient sock = null;
		private static BinaryWriter sock_writer = null;
		private static BinaryReader sock_reader = null;
		private static Bitmap bitmap = null;
		private int nWidth, nHeight;
		private static int nNumColors = 100;
		private static int nMax = 100;
		private static Color [] colors = null;
		private static double xmin = -1.0, ymin = -1.0, xmax = 1.0, ymax = 1.0;
		private static bool bDrawing = false;
		private static PictureBox pBox = null;
		private Point p1, p2;
		private Thread thread = null;
		private System.Windows.Forms.Button demo_button;
		private System.Windows.Forms.ComboBox points_comboBox;
		private Rectangle rBox;
		private System.Windows.Forms.Button go_stop_button;
		private static ArrayList demo_list = null;
		private static bool bDemoMode = false;
		private static int demo_iter = 0;
		private static MandelViewerForm form = null;

		static void work_thread()
		{
			int [] temp = new int[4];
			int [] buffer = null;
			int size;
			int i, j, k;
			Graphics g;

			do
			{
				try
				{
					g = Graphics.FromImage(bitmap);
					g.Clear(Color.Black);
					g.Dispose();
					g = null;
					pBox.Invalidate();

					if (bDemoMode)
					{
						if (demo_list != null)
						{
							if (demo_list.Count > demo_iter)
							{
								xmin = ((ExamplePoint)demo_list[demo_iter]).xmin;
								ymin = ((ExamplePoint)demo_list[demo_iter]).ymin;
								xmax = ((ExamplePoint)demo_list[demo_iter]).xmax;
								ymax = ((ExamplePoint)demo_list[demo_iter]).ymax;
								nMax = ((ExamplePoint)demo_list[demo_iter]).max_iter;
								if (((ExamplePoint)demo_list[demo_iter]).name != null)
									form.Text = "Mandelbrot Viewer - " + ((ExamplePoint)demo_list[demo_iter]).name;
								else
									form.Text = String.Format("Mandelbrot Viewer - {0}", demo_iter);
								colors = new Color[nMax+1];
								ColorRainbow.Make_color_array(nNumColors, colors);
								colors[nMax] = Color.FromKnownColor(KnownColor.Black); // add one on the top to avoid edge errors
							}
							demo_iter++;
							if (demo_iter >= demo_list.Count)
								demo_iter = 0;
						}
					}

					if (colors.Length != nMax + 1)
					{
						colors = new Color[nMax+1];
						ColorRainbow.Make_color_array(nNumColors, colors);
						colors[nMax] = Color.FromKnownColor(KnownColor.Black); // add one on the top to avoid edge errors
					}
					sock_writer.Write(xmin);
					sock_writer.Write(ymin);
					sock_writer.Write(xmax);
					sock_writer.Write(ymax);
					sock_writer.Write(nMax);

					for (;;)
					{
						temp[0] = sock_reader.ReadInt32();
						temp[1] = sock_reader.ReadInt32();
						temp[2] = sock_reader.ReadInt32();
						temp[3] = sock_reader.ReadInt32();
						if (temp[0] == 0 && temp[1] == 0 && temp[2] == 0 && temp[3] == 0)
						{
							if (bDemoMode)
							{
								Thread.Sleep(5000);
								break;
							}
							bDrawing = false;
							return;
						}

						size = (temp[1] + 1 - temp[0]) * (temp[3] + 1 - temp[2]);
						buffer = new int[size];

						for (i=0; i<size; i++)
							buffer[i] = sock_reader.ReadInt32();

						int max_color = colors.Length;
						Random rand = new Random();
						try
						{
							lock (bitmap)
							{
								k = 0;
								for (j=temp[2]; j<=temp[3]; j++)
								{
									for (i=temp[0]; i<=temp[1]; i++)
									{
										//bitmap.SetPixel(i, j, colors[buffer[k]]);
										
										if (buffer[k] >= 0 && buffer[k] < max_color)
											bitmap.SetPixel(i, j, colors[buffer[k]]);
										else
										{
											bitmap.SetPixel(i, j, Color.FromArgb(rand.Next(0, 255), rand.Next(0, 255), rand.Next(0,255)));
										}
										
										k++;
									}
								}
							}
						}
						catch (Exception e)
						{
							MessageBox.Show("exception thrown while accessing bitmap: " + e.Message, "Error");
						}
						pBox.Invalidate();
					}
				}
				catch (Exception e)
				{
					// do something with the exception
					MessageBox.Show("Exception thrown in worker thread: " + e.Message, "Error");
					break;
				}
			} while (bDemoMode);
			bDrawing = false;
		}

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public MandelViewerForm()
		{
			InitializeComponent();

			try
			{
				// This throws a security exception if you are on a network share or
				// running from the web.
				host.Text = System.Environment.MachineName;
			} 
			catch (Exception)
			{
				host.Text = "localhost";
			}
			port.Text = "7470";
			p1 = new Point(0,0);
			p2 = new Point(0,0);
			rBox = new Rectangle(0,0,0,0);
			go_stop_button.Enabled = false;
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
			this.connect_button = new System.Windows.Forms.Button();
			this.host = new System.Windows.Forms.TextBox();
			this.port = new System.Windows.Forms.TextBox();
			this.outputBox = new System.Windows.Forms.PictureBox();
			this.demo_button = new System.Windows.Forms.Button();
			this.points_comboBox = new System.Windows.Forms.ComboBox();
			this.go_stop_button = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// connect_button
			// 
			this.connect_button.Location = new System.Drawing.Point(8, 8);
			this.connect_button.Name = "connect_button";
			this.connect_button.Size = new System.Drawing.Size(56, 23);
			this.connect_button.TabIndex = 0;
			this.connect_button.Text = "Connect";
			this.connect_button.Click += new System.EventHandler(this.connect_button_Click);
			// 
			// host
			// 
			this.host.Location = new System.Drawing.Point(72, 8);
			this.host.Name = "host";
			this.host.TabIndex = 1;
			this.host.Text = "host";
			// 
			// port
			// 
			this.port.Location = new System.Drawing.Point(176, 8);
			this.port.Name = "port";
			this.port.Size = new System.Drawing.Size(40, 20);
			this.port.TabIndex = 2;
			this.port.Text = "7470";
			// 
			// outputBox
			// 
			this.outputBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.outputBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.outputBox.Location = new System.Drawing.Point(8, 40);
			this.outputBox.Name = "outputBox";
			this.outputBox.Size = new System.Drawing.Size(760, 760);
			this.outputBox.TabIndex = 3;
			this.outputBox.TabStop = false;
			this.outputBox.Paint += new System.Windows.Forms.PaintEventHandler(this.outputBox_Paint);
			this.outputBox.MouseUp += new System.Windows.Forms.MouseEventHandler(this.outputBox_MouseUp);
			this.outputBox.MouseMove += new System.Windows.Forms.MouseEventHandler(this.outputBox_MouseMove);
			this.outputBox.MouseDown += new System.Windows.Forms.MouseEventHandler(this.outputBox_MouseDown);
			// 
			// demo_button
			// 
			this.demo_button.Location = new System.Drawing.Point(224, 8);
			this.demo_button.Name = "demo_button";
			this.demo_button.Size = new System.Drawing.Size(88, 23);
			this.demo_button.TabIndex = 4;
			this.demo_button.Text = "Demo Points";
			this.demo_button.Click += new System.EventHandler(this.demo_button_Click);
			// 
			// points_comboBox
			// 
			this.points_comboBox.Location = new System.Drawing.Point(320, 8);
			this.points_comboBox.MaxDropDownItems = 20;
			this.points_comboBox.Name = "points_comboBox";
			this.points_comboBox.Size = new System.Drawing.Size(400, 21);
			this.points_comboBox.TabIndex = 5;
			this.points_comboBox.Text = "points";
			this.points_comboBox.DropDown += new System.EventHandler(this.points_comboBox_DropDown);
			this.points_comboBox.SelectedValueChanged += new System.EventHandler(this.points_comboBox_SelectedValueChanged);
			// 
			// go_stop_button
			// 
			this.go_stop_button.Location = new System.Drawing.Point(728, 8);
			this.go_stop_button.Name = "go_stop_button";
			this.go_stop_button.Size = new System.Drawing.Size(40, 23);
			this.go_stop_button.TabIndex = 6;
			this.go_stop_button.Text = "Go";
			this.go_stop_button.Click += new System.EventHandler(this.go_stop_button_Click);
			// 
			// MandelViewerForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(776, 811);
			this.Controls.Add(this.go_stop_button);
			this.Controls.Add(this.points_comboBox);
			this.Controls.Add(this.demo_button);
			this.Controls.Add(this.outputBox);
			this.Controls.Add(this.port);
			this.Controls.Add(this.host);
			this.Controls.Add(this.connect_button);
			this.Name = "MandelViewerForm";
			this.Text = "Mandelbrot Viewer";
			this.Resize += new System.EventHandler(this.MandelViewerForm_Resize);
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			double d = 0.0;
			int i = 0;
			form = new MandelViewerForm();
			Application.Run(form);
			try
			{
				// tell the mpi program to stop
				MandelViewerForm.sock_writer.Write(d);
				MandelViewerForm.sock_writer.Write(d);
				MandelViewerForm.sock_writer.Write(d);
				MandelViewerForm.sock_writer.Write(d);
				MandelViewerForm.sock_writer.Write(i);
			}
			catch (Exception)
			{
				// Do nothing.  It just means that the socket connection to the pmandel program is broken.
			}
		}

		private void connect_button_Click(object sender, System.EventArgs e)
		{
			if (bConnected)
			{
				MessageBox.Show("You may only connect once", "Note");
				return;
			}

			try
			{
				sock = new TcpClient(host.Text, Convert.ToInt32(port.Text));
				if (sock == null)
				{
					MessageBox.Show("Unable to connect to " + host.Text + " on port " + port.Text, "Error");
					return;
				}
			}
			catch(SocketException exception)
			{
				MessageBox.Show("Unable to connect to " + host.Text + " on port " + port.Text + ", " + exception.Message, "Error");
				return;
			}
			bConnected = true;
			sock_writer = new BinaryWriter(sock.GetStream());
			sock_reader = new BinaryReader(sock.GetStream());

			nWidth = sock_reader.ReadInt32();
			nHeight = sock_reader.ReadInt32();
			nNumColors = sock_reader.ReadInt32();
			nMax = sock_reader.ReadInt32();
			//MessageBox.Show(String.Format("Width: {0}, Height: {1}, num_colors: {2}", nWidth, nHeight, nNumColors), "Note");
			// validate input values
			if (nWidth > 2048)
				nWidth = 2048;
			if (nWidth < 1)
				nWidth = 768;
			if (nHeight > 2048)
				nHeight = 2048;
			if (nHeight < 1)
				nHeight = 768;
			if (nNumColors > 1024)
				nNumColors = 1024;
			if (nNumColors < 1)
				nNumColors = 128;
			if (nMax < 1)
				nMax = 100;
			if (nMax > 5000)
				nMax = 5000;
			colors = new Color[nMax+1];
			ColorRainbow.Make_color_array(nNumColors, colors);
			colors[nMax] = Color.FromKnownColor(KnownColor.Black); // add one on the top to avoid edge errors
			bitmap = new Bitmap(nWidth, nHeight, System.Drawing.Imaging.PixelFormat.Format24bppRgb);
			Graphics g;
			g = Graphics.FromImage(bitmap);
			g.Clear(Color.FromKnownColor(KnownColor.Black));
			g.Dispose();

			/*
			Rectangle rButton = connect_button.Bounds;
			Rectangle rBox = outputBox.Bounds;
			outputBox.SetBounds(rButton.Left, rButton.Top, rBox.Width, rBox.Height + (rBox.Top - rButton.Top));
			connect_button.Hide();
			host.Hide();
			port.Hide();
			outputBox.Invalidate();
			*/
			connect_button.Enabled = false;
			host.ReadOnly = true;
			port.ReadOnly = true;
			if (demo_list != null && demo_list.Count > 0)
			{
				go_stop_button.Enabled = true;
			}

			bDrawing = true;
			pBox = outputBox;
			ThreadStart threadProc = new ThreadStart(work_thread);
		    thread = new Thread(threadProc);
			thread.Start();
		}

		private void outputBox_Paint(object sender, System.Windows.Forms.PaintEventArgs e)
		{
			if (bConnected && bitmap != null)
			{
				lock (bitmap)
				{
					e.Graphics.DrawImage(bitmap, 0, 0, outputBox.Size.Width, outputBox.Size.Height);
					if (rBox.Width > 0 && rBox.Height > 0)
					{
						SolidBrush brush = new SolidBrush(Color.FromArgb(198,255,255,0));
						e.Graphics.FillRectangle(brush, rBox);
					}
				}
			}
		}

		private void MandelViewerForm_Resize(object sender, System.EventArgs e)
		{
			if (bConnected)
			{
				outputBox.Invalidate();
			}
		}

		private void outputBox_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (!bDrawing && e.Button == MouseButtons.Left)
			{
				p1 = new Point(e.X, e.Y);
				rBox = new Rectangle(p1.X, p1.Y, 0, 0);
			}
		}

		private void outputBox_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (!bDrawing && e.Button == MouseButtons.Left)
			{
				int x, y;
				p2 = new Point(e.X, e.Y);
				x = Math.Min(p1.X, p2.X);
				y = Math.Min(p1.Y, p2.Y);
				rBox = new Rectangle(x, y, Math.Max(p1.X, p2.X) - x, Math.Max(p1.Y, p2.Y) - y);
				outputBox.Invalidate();
			}
		}

		private void outputBox_MouseUp(object sender, System.Windows.Forms.MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Left)
			{
				if (!bDrawing && thread != null)
				{
					Rectangle r;
					double x1,y1,x2,y2;
					double width, height, pixel_width, pixel_height;
					int x, y;

					if (p1.X == e.X && p1.Y == e.Y)
					{
						return;
					}

					p2 = new Point(e.X, e.Y);
					x = Math.Min(p1.X, p2.X);
					y = Math.Min(p1.Y, p2.Y);

					thread.Join();

					r = new Rectangle(x, y, Math.Max(p1.X, p2.X) - x, Math.Max(p1.Y, p2.Y) - y);
					width = xmax - xmin;
					height = ymax - ymin;
					pixel_width = outputBox.Width;
					pixel_height = outputBox.Height;
					x1 = xmin + ((double)r.Left * width / pixel_width);
					x2 = xmin + ((double)r.Right * width / pixel_width);
					y2 = ymin + ((double)(pixel_height - r.Top) * height / pixel_height);
					y1 = ymin + ((double)(pixel_height - r.Bottom) * height / pixel_height);
					xmin = x1;
					xmax = x2;
					ymin = y1;
					ymax = y2;

					ExamplePoint ep = new ExamplePoint(xmin, ymin, xmax, ymax, nMax, "dragged");
					Text = "Mandelbrot Viewer" + ep.ToShortString();

					bDrawing = true;
					ThreadStart threadProc = new ThreadStart(work_thread);
					thread = new Thread(threadProc);
					thread.Start();

					rBox = new Rectangle(x, y, 0, 0);
					outputBox.Invalidate();
				}
			}
			if (e.Button == MouseButtons.Right)
			{
				if (!bDrawing && thread != null)
				{
					thread.Join();

					xmin = -1.0;
					xmax = 1.0;
					ymin = -1.0;
					ymax = 1.0;

					bDrawing = true;
					ThreadStart threadProc = new ThreadStart(work_thread);
					thread = new Thread(threadProc);
					thread.Start();

					rBox = new Rectangle(0, 0, 0, 0);
					outputBox.Invalidate();
				}
			}
		}

		private void demo_button_Click(object sender, System.EventArgs e)
		{
			if (demo_list == null)
				demo_list = new ArrayList();
			OpenFileDialog dlg = new OpenFileDialog();

			dlg.InitialDirectory = "c:\\";
			dlg.Filter = "txt files (*.txt)|*.txt|All files (*.*)|*.*";
			dlg.FilterIndex = 1;
			dlg.RestoreDirectory = true;

			if(dlg.ShowDialog() == DialogResult.OK)
			{
				using (StreamReader sr = new StreamReader(dlg.OpenFile()))
				{
					String line;
					bool first = true;

					while ((line = sr.ReadLine()) != null) 
					{
						line.Trim();
						if (line.Length > 0 && line[0] != '#')
						{
							ExamplePoint node = new ExamplePoint();
							if (node.FromString(line))
							{
								if (first)
								{
									demo_list.Clear(); // new list replaces the old
									first = false;
								}
								demo_list.Add(node);
							}
						}
					}
					sr.Close();

					points_comboBox.Items.Clear();
					for (int i=0; i<demo_list.Count; i++)
					{
						//MessageBox.Show(((ExamplePoint)demo_list[i]).ToString(), String.Format("demo: {0}", i));
						points_comboBox.Items.Add(((ExamplePoint)demo_list[i]).ToString());
					}
					points_comboBox.SelectedIndex = 0;
					if (connect_button.Enabled == false)
					{
						go_stop_button.Enabled = true;
					}
				}
			}
		}

		private void go_stop_button_Click(object sender, System.EventArgs e)
		{
			if (bDemoMode)
			{
				go_stop_button.Text = "Go";
				bDemoMode = false;
			}
			else
			{
				go_stop_button.Text = "Stop";
				bDemoMode = true;
				if (!bDrawing && thread != null)
				{
					thread.Join();

					bDrawing = true;
					ThreadStart threadProc = new ThreadStart(work_thread);
					thread = new Thread(threadProc);
					thread.Start();

					rBox = new Rectangle(0, 0, 0, 0);
					outputBox.Invalidate();
				}
			}
		}

		private void points_comboBox_SelectedValueChanged(object sender, System.EventArgs e)
		{
			if (!bDrawing && thread != null)
			{
				thread.Join();

				ExamplePoint p = new ExamplePoint();
				if (p.FromString(points_comboBox.SelectedItem.ToString()))
				{
					xmin = p.xmin;
					xmax = p.xmax;
					ymin = p.ymin;
					ymax = p.ymax;
					nMax = p.max_iter;

					if (p.name != null && p.name != "")
						Text = "Mandelbrot Viewer - " + p.name;
					else
						Text = "Mandelbrot Viewer";

					bDrawing = true;
					ThreadStart threadProc = new ThreadStart(work_thread);
					thread = new Thread(threadProc);
					thread.Start();

					rBox = new Rectangle(0, 0, 0, 0);
					outputBox.Invalidate();
				}
			}
		}

		private void points_comboBox_DropDown(object sender, System.EventArgs e)
		{
			ComboBox senderComboBox = (ComboBox)sender;
			int width = senderComboBox.DropDownWidth;
			Graphics g = senderComboBox.CreateGraphics();
			Font font = senderComboBox.Font;
			int vertScrollBarWidth = 0;

			if (senderComboBox.Items.Count>senderComboBox.MaxDropDownItems)
				vertScrollBarWidth = SystemInformation.VerticalScrollBarWidth;

			int newWidth;
			foreach (string s in ((ComboBox)sender).Items)
			{
				newWidth = (int) g.MeasureString(s, font).Width 
					+ vertScrollBarWidth;
				if (width < newWidth )
				{
					width = newWidth;
				}
			}
			senderComboBox.DropDownWidth = width;		
		}
	}

	public class ExamplePoint
	{
		public double xmin;
		public double ymin;
		public double xmax;
		public double ymax;
		public int max_iter;
		public string name;

		public ExamplePoint()
		{
			xmin = -1;
			xmax = 1;
			ymin = -1;
			ymax = 1;
			max_iter = 100;
			name = null;
		}
		public ExamplePoint(double x0, double y0, double x1, double y1, int m)
		{
			xmin = x0;
			ymin = y0;
			xmax = x1;
			ymax = y1;
			max_iter = m;
			name = null;
		}
		public ExamplePoint(double x0, double y0, double x1, double y1, int m, string n)
		{
			xmin = x0;
			ymin = y0;
			xmax = x1;
			ymax = y1;
			max_iter = m;
			name = n;
		}
		public ExamplePoint(string s)
		{
			xmin = -1;
			xmax = 1;
			ymin = -1;
			ymax = 1;
			max_iter = 100;
			name = null;
			FromString(s);
		}
		public bool FromString(string s)
		{
			double xcenter = 0;
			double ycenter = 0;
			double radius = 0;
			xmin = -2;
			xmax = 2;
			ymin = -2;
			ymax = 2;
			max_iter = 100;
			name = null;
			bool bxmin=false, bxmax=false, bymin=false, bymax=false;

			int index = 0;
			string [] ss;
			int n_range = 0;
			StringArgParser p = new StringArgParser();
			ss = p.ParseStrings(s);
			if (ss != null && ss.Length > 0)
			{
				while (index < ss.Length)
				{
					string str;
					str = ss[index];
					str.Trim();
					index++;
					if (str.Length > 1 && str[0] == '(' && str[str.Length-1] == ')')
					{
						double x,y;
						string inside = str.Substring(1, str.Length-2);
						if (inside != null && inside.Length > 0)
						{
							string [] range_strings;
							range_strings = inside.Split(',');
							if (range_strings != null && range_strings.Length == 2)
							{
								x = Convert.ToDouble(range_strings[0]);
								y = Convert.ToDouble(range_strings[1]);
								if (n_range == 0)
								{
									xmin = x;
									ymin = y;
									bxmin = bymin = true;
								}
								else if (n_range == 1)
								{
									xmax = x;
									ymax = y;
									bxmax = bymax = true;
								}
								n_range++;
							}
						}
					}
					else if (str == "-rmin")
					{
						if (index < ss.Length)
						{
							xmin = Convert.ToDouble(ss[index]);
							bxmin = true;
							index++;
						}
					}
					else if (str == "-rmax")
					{
						if (index < ss.Length)
						{
							xmax = Convert.ToDouble(ss[index]);
							bxmax = true;
							index++;
						}
					}
					else if (str == "-imin")
					{
						if (index < ss.Length)
						{
							ymin = Convert.ToDouble(ss[index]);
							bymin = true;
							index++;
						}
					}
					else if (str == "-imax")
					{
						if (index < ss.Length)
						{
							ymax = Convert.ToDouble(ss[index]);
							bymax = true;
							index++;
						}
					}
					else if (str == "-rcenter")
					{
						if (index < ss.Length)
						{
							xcenter = Convert.ToDouble(ss[index]);
							index++;
						}
					}
					else if (str == "-icenter")
					{
						if (index < ss.Length)
						{
							ycenter = Convert.ToDouble(ss[index]);
							index++;
						}
					}
					else if (str == "-radius")
					{
						if (index < ss.Length)
						{
							radius = Convert.ToDouble(ss[index]);
							index++;
						}
					}
					else if (str == "max_iter" || str == "-max_iter" || str == "-maxiter")
					{
						if (index < ss.Length)
						{
							max_iter = Convert.ToInt32(ss[index]);
							index++;
						}
					}
					else if (str == "name" || str == "-name")
					{
						if (index < ss.Length)
						{
							name = ss[index];
							index++;
						}
					}
				}
			}

			if (radius != 0)
			{
				xmin = xcenter - radius;
				xmax = xcenter + radius;
				ymin = ycenter - radius;
				ymax = ycenter + radius;
				bxmin = bymin = bxmax = bymax = true;
			}

			if (xmin > xmax)
			{
				double d;
				d = xmin;
				xmin = xmax;
				xmax = d;
			}
			if (ymin > ymax)
			{
				double d;
				d = ymin;
				ymin = ymax;
				ymax = d;
			}
			if (xmin < -2)
				xmin = -2;
			if (xmax > 2)
				xmax = 2;
			if (ymin < -2)
				ymin = -2;
			if (ymax > 2)
				ymax = 2;
			if (max_iter < 1)
				max_iter = 100;
			if (max_iter > 10000)
				max_iter = 100;
			if (bxmin == true && bymin == true && bxmax == true && bymax == true)
				return true;
			return false;
		}
		public override string ToString()
		{
			if (name == null)
			{
				return String.Format("({0},{1}) ({2},{3}) max_iter {4}",
					xmin, ymin, xmax, ymax, max_iter);
			}
			else
			{
				return String.Format("({0},{1}) ({2},{3}) max_iter {4} name \"{5}\"",
					xmin, ymin, xmax, ymax, max_iter, name);
			}
		}
		public string ToShortString()
		{
			if (name == null)
			{
				return String.Format("({0},{1}) ({2},{3}) max_iter {4}",
					xmin.ToString("#.##E0"), ymin.ToString("#.##E0"), xmax.ToString("#.##E0"), ymax.ToString("#.##E0"), max_iter);
			}
			else
			{
				return String.Format("({0},{1}) ({2},{3}) max_iter {4} name \"{5}\"",
					xmin.ToString("#.##E0"), ymin.ToString("#.##E0"), xmax.ToString("#.##E0"), ymax.ToString("#.##E0"), max_iter, name);
			}
		}
	}

	public class ColorRainbow
	{
		static public Color getColor(double fraction, double intensity)
		{
			/* fraction is a part of the rainbow (0.0 - 1.0) = (Red-Yellow-Green-Cyan-Blue-Magenta-Red)
			intensity (0.0 - 1.0) 0 = black, 1 = full color, 2 = white
			*/
			double red, green, blue;
			int r,g,b;
			double dtemp;

			//fraction = Math.Abs(modf(fraction, &dtemp));
			fraction = Math.Abs(fraction - Math.Floor(fraction));

			if (intensity > 2.0)
				intensity = 2.0;
			if (intensity < 0.0)
				intensity = 0.0;

			dtemp = 1.0/6.0;

			if (fraction < 1.0/6.0)
			{
				red = 1.0;
				green = fraction / dtemp;
				blue = 0.0;
			}
			else
			{
				if (fraction < 1.0/3.0)
				{
					red = 1.0 - ((fraction - dtemp) / dtemp);
					green = 1.0;
					blue = 0.0;
				}
				else
				{
					if (fraction < 0.5)
					{
						red = 0.0;
						green = 1.0;
						blue = (fraction - (dtemp*2.0)) / dtemp;
					}
					else
					{
						if (fraction < 2.0/3.0)
						{
							red = 0.0;
							green = 1.0 - ((fraction - (dtemp*3.0)) / dtemp);
							blue = 1.0;
						}
						else
						{
							if (fraction < 5.0/6.0)
							{
								red = (fraction - (dtemp*4.0)) / dtemp;
								green = 0.0;
								blue = 1.0;
							}
							else
							{
								red = 1.0;
								green = 0.0;
								blue = 1.0 - ((fraction - (dtemp*5.0)) / dtemp);
							}
						}
					}
				}
			}

			if (intensity > 1)
			{
				intensity = intensity - 1.0;
				red = red + ((1.0 - red) * intensity);
				green = green + ((1.0 - green) * intensity);
				blue = blue + ((1.0 - blue) * intensity);
			}
			else
			{
				red = red * intensity;
				green = green * intensity;
				blue = blue * intensity;
			}

			r = (int)(red * 255.0);
			g = (int)(green * 255.0);
			b = (int)(blue * 255.0);

			return Color.FromArgb(r,g,b);
		}

		public static void Make_color_array(int num_colors, Color[] colors)
		{
			double fraction, intensity;
			int i;
			int max;

			max = colors.Length;
			intensity = 1.0;
			for (i=0; i<max; i++)
			{
				fraction = (double)(i % num_colors) / (double)num_colors;
				colors[i] = getColor(fraction, intensity);
			}
		}
	}
	public class StringArgParser
	{
		public string [] ParseStrings(string str)
		{
			string [] stemp;
			string [] return_strings;
			StringCollection s = new StringCollection();
			if (str.Length < 1)
				return null;
			while (str != null)
			{
				stemp = ParseString(str);
				if (stemp != null)
				{
					s.Add(stemp[0]);
					str = stemp[1];
				}
				else
				{
					str = null;
				}
			}
			return_strings = new string[s.Count];
			for (int i=0; i<s.Count; i++)
				return_strings[i] = s[i];
			return return_strings;
		}
		string [] ParseString(string str)
		{
			int index = 0;
			StringBuilder s = new StringBuilder();
			String s2;
			string [] return_string;

			if (str == null)
				return null;
			while (index < str.Length && str[index] == ' ')
				index++;
			if (index == str.Length)
				return null;
			if (str[index] == '"')
			{
				// parse quoted
				index++; // step over the first quote character
				while (index < str.Length && str[index] != '"')
				{
					s.Append(str[index]);
					index++;
				}
				if (index < str.Length)
					index++; // step over the second quote
			}
			else
			{
				// parse literal
				while (index < str.Length && str[index] != ' ')
				{
					s.Append(str[index]);
					index++;
				}
			}
			while (index < str.Length && str[index] == ' ')
				index++;
			if (index < str.Length)
				s2 = str.Substring(index, str.Length - index);
			else
				s2 = null;
			return_string = new string[2];
			return_string[0] = s.ToString();
			return_string[1] = s2;
			return return_string;
		}
	}
}
