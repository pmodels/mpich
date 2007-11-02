/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <math.h>

#define RGBtocolor_t(r,g,b) ((color_t)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define getR(r) ((int)((r) & 0xFF))
#define getG(g) ((int)((g>>8) & 0xFF))
#define getB(b) ((int)((b>>16) & 0xFF))

typedef int color_t;

int send_xyminmax(double xmin, double ymin, double xmax, double ymax);
int read_data(SOCKET sock, char *buffer, int length);

extern HWND g_hWnd;
extern HDC g_hDC;
extern int g_width, g_height;
extern HANDLE g_hMutex;
int g_num_colors;
static int g_max_iter;
color_t *colors;

SOCKET sock;

color_t getColor(double fraction, double intensity)
{
    /* fraction is a part of the rainbow (0.0 - 1.0) = (Red-Yellow-Green-Cyan-Blue-Magenta-Red)
    intensity (0.0 - 1.0) 0 = black, 1 = full color, 2 = white
    */
    double red, green, blue;
    int r,g,b;
    double dtemp;

    fraction = fabs(modf(fraction, &dtemp));

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

    return RGBtocolor_t(r,g,b);
}

int Make_color_array(int num_colors, int max, color_t colors[])
{
    double fraction, intensity;
    int i;

    intensity = 1.0;
    for (i=0; i<max; i++)
    {
	fraction = (double)(i % num_colors) / (double)num_colors;
	colors[i] = getColor(fraction, intensity);
    }
    return 0;
}

void getRGB(color_t color, int *r, int *g, int *b)
{
    *r = getR(color);
    *g = getG(color);
    *b = getB(color);
}

int connect_to_pmandel(const char *host, int port, int &width, int &height)
{
    sockaddr_in addr;
    int result;
    static int started = FALSE;
    WSADATA wsaData;
    char err[100];
    struct hostent *lphost;
    int optval;

    if (!started)
    {
	/* Start the Winsock dll */
	if ((result = WSAStartup(MAKEWORD(2, 0), &wsaData)) != 0)
	{
	    return result;
	}
	started = TRUE;
    }
    addr.sin_family = AF_INET;
    lphost = gethostbyname(host);
    if (lphost != NULL)
	addr.sin_addr.s_addr = ((struct in_addr *)lphost->h_addr)->s_addr;
    else
    {
	sprintf(err, "gethostbyname failed: error %d", WSAGetLastError());
	MessageBox(NULL, err, "Error", MB_OK);
	return -1;
    }
    addr.sin_port = htons(port);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
	sprintf(err, "connect failed: error %d", WSAGetLastError());
	MessageBox(NULL, err, "Error", MB_OK);
	return -1;
    }
    optval = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval));
    result = read_data(sock, (char*)&width, sizeof(int));
    if (result)
	return -1;
    result = read_data(sock, (char*)&height, sizeof(int));
    if (result)
	return -1;
    result = read_data(sock, (char*)&g_num_colors, sizeof(int));
    if (result)
	return -1;
    result = read_data(sock, (char*)&g_max_iter, sizeof(int));
    if (result)
	return -1;
    g_width = width;
    g_height = height;
    colors = new color_t[g_max_iter+1];
    Make_color_array(g_num_colors, g_max_iter, colors);
    colors[g_max_iter] = 0; /* add one on the top to avoid edge errors */
    /*
    sprintf(err, "num_colors = %d", g_num_colors);
    MessageBox(NULL, err, "Note", MB_OK);
    */
    return 0;
}

int send_xyminmax(double xmin, double ymin, double xmax, double ymax, int max_iter)
{
    double temp[4];
    int result, total;
    char err[100];
    RECT r;
    char *buffer;

    if (xmin != xmax && ymin != ymax && g_hDC)
    {
	r.left = 0;
	r.right = g_width;
	r.top = 0;
	r.bottom = g_height;
	FillRect(g_hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    total = 4 * sizeof(double);
    temp[0] = xmin;
    temp[1] = ymin;
    temp[2] = xmax;
    temp[3] = ymax;
    buffer = (char *)temp;
    while (total)
    {
	result = send(sock, (const char *)buffer, total, 0);
	if (result == SOCKET_ERROR)
	{
	    result = WSAGetLastError();
	    if (result == WSAEWOULDBLOCK)
	    {
		timeval t;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, NULL, &set, NULL, &t);
		continue;
	    }
	    sprintf(err, "send failed: error %d", result);
	    MessageBox(NULL, err, "Error", MB_OK);
	    return -1;
	}
	total -= result;
	buffer += result;
    }

    total = sizeof(int);
    buffer = (char*)&max_iter;
    while (total)
    {
	result = send(sock, (const char *)buffer, total, 0);
	if (result == SOCKET_ERROR)
	{
	    result = WSAGetLastError();
	    if (result == WSAEWOULDBLOCK)
	    {
		timeval t;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, NULL, &set, NULL, &t);
		continue;
	    }
	    sprintf(err, "send failed: error %d", result);
	    MessageBox(NULL, err, "Error", MB_OK);
	    return -1;
	}
	total -= result;
	buffer += result;
    }

    if (max_iter != g_max_iter)
    {
	if (colors)
	    delete [] colors;
	g_max_iter = max_iter;
	colors = new color_t[g_max_iter+1];
	Make_color_array(g_num_colors, g_max_iter, colors);
	colors[g_max_iter] = 0; /* add one on the top to avoid edge errors */
    }
    return 0;
}

int read_data(SOCKET sock, char *buffer, int length)
{
    int result;
    int num_bytes;
    char err[100];

    while (length)
    {
	num_bytes = recv(sock, buffer, length, 0);
	if (num_bytes == SOCKET_ERROR)
	{
	    result = WSAGetLastError();
	    if (result == WSAEWOULDBLOCK)
	    {
		timeval t;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(sock, &set);
		t.tv_sec = 1;
		t.tv_usec = 0;
		select(1, &set, NULL, NULL, &t);
		continue;
	    }
	    sprintf(err, "recv failed: error %d", result);
	    MessageBox(NULL, err, "Error", MB_OK);
	    return result;
	}
	if (num_bytes == 0)
	{
	    MessageBox(NULL, "socket connection to pmandel closed", "Error", MB_OK);
	    return SOCKET_ERROR;
	}
	length -= num_bytes;
	buffer += num_bytes;
    }
    return 0;
}

int get_pmandel_data()
{
    int temp[4];
    int *buffer;
    int size;
    int result;
    int i, j, k;
    RECT r;

    for (;;)
    {
	result = read_data(sock, (char*)temp, 4*sizeof(int));
	if (result)
	    return 0;
	if (temp[0] == 0 && temp[1] == 0 && temp[2] == 0 && temp[3] == 0)
	    return 0;
	size = (temp[1] + 1 - temp[0]) * (temp[3] + 1 - temp[2]);
	buffer = new int[size];
	result = read_data(sock, (char *)buffer, size * sizeof(int));
	if (result)
	{
	    delete buffer;
	    return 0;
	}
	WaitForSingleObject(g_hMutex, INFINITE);
	k = 0;
	for (j=temp[2]; j<=temp[3]; j++)
	{
	    for (i=temp[0]; i<=temp[1]; i++)
	    {
		//SetPixel(g_hDC, i, j, colors[buffer[k]]);
		SetPixelV(g_hDC, i, j, colors[buffer[k]]);
		k++;
	    }
	}
	ReleaseMutex(g_hMutex);
	r.left = 0;
	r.right = g_width;
	r.top = 0;
	r.bottom = g_height;
	InvalidateRect(g_hWnd, &r, FALSE);
	delete buffer;
    }
    return 1;
}
