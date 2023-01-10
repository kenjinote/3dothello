#define _WIN32_WINNT 0x0400
#define UNICODE
#pragma comment(linker,"/opt:nowin98")
#pragma comment(lib,"opengl32")
#pragma comment(lib,"glu32")
#include<windows.h>
#include<stdio.h>
#include<gl\gl.h>
#include<gl\Glu.h>
#include"resource.h"

extern "C" int _fltused=1;

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define WINDOW_ASPECT ((float)WINDOW_HEIGHT/(float)WINDOW_WIDTH)

HDC hDC;
HGLRC hRC;
BOOL active=TRUE;
TCHAR szClassName[]=TEXT("Window");
GLfloat rot[2];
GLfloat trans;

#define MASU_SIZE   (2.0f/8.0f)            //マス目（石）のサイズ
#define MASU_NUM    8                        //マス目の数（１方向に対する）
#define BLACK_STONE   1                      //黒石
#define WHITE_STONE  -1                      //白石
#define BLANK         0                      //石なし
#define END_NUMBER   60                      //オセロ終了の手数
#define SEARCH_LV     5                      //探索する手数
#define MIDDLE_NUM   10                      //中盤の始まる手数
#define FINISH_NUM   48                      //終盤の始まる手数
#define WM_ENDTHREAD (WM_APP+100)

const int ValuePlace[MASU_NUM][MASU_NUM]=
{
	{ 45,-11,4,-1,-1,4,-11,45,},
	{-11,-16,-1,-3,-3,-1,-16,-11,},
	{  4,-1,2,-1,-1,2,-1,4,},
	{ -1,-3,-1,0,0,-1,-3,-1,},
	{ -1,-3,-1,0,0,-1,-3,-1,},
	{  4,-1,2,-1,-1,2,-1,4,},
	{-11,-16,-1,-3,-3,-1,-16,-11,},
	{ 45,-11,4,-1,-1,4,-11,45,}
};

typedef struct reverse_info
{
	int x,y;
	int pointer;
	int position[30];
}Ando;

char m_BoardDisplay[MASU_NUM][MASU_NUM];
//char m_BoardBackMemory[MASU_NUM][MASU_NUM];
int m_PutNumber;
int m_SearchLv;
BOOL m_FlagForWhite;
BOOL m_FlagForPlayer;
BOOL m_FlagInGame;
HWND hWnd;

int CountStone(int stone)
{
	int x,y,count=0;
	for(x=0;x<MASU_NUM;x++)
		for(y=0;y<MASU_NUM;y++)
			if(m_BoardDisplay[x][y]==stone)
				count++;
	return(count);
}

void End()
{
	if(m_PutNumber==END_NUMBER)
	{
		int num;
		if(m_FlagForPlayer)
			num=CountStone(WHITE_STONE);
		else
			num=CountStone(BLACK_STONE);
		m_FlagInGame=FALSE;
		if(num*2>(m_PutNumber+4))
			MessageBox(
			hWnd,
			TEXT("あなたの勝ちです"),
			TEXT("勝敗"),
			0);
		else if(num*2<(m_PutNumber+4))
			MessageBox(
			hWnd,
			TEXT("あなたの負けです"),
			TEXT("勝敗"),
			0);
		else 
			MessageBox(
			hWnd,
			TEXT("引き分けです"),
			TEXT("勝敗"),
			0);
	}
}

void InitBoard()
{
	int x,y;
	for(x=0;x<MASU_NUM;x++)
		for(y=0;y<MASU_NUM;y++)
			m_BoardDisplay[x][y]=BLANK;
	m_BoardDisplay[3][3]=m_BoardDisplay[4][4]=WHITE_STONE;
	m_BoardDisplay[3][4]=m_BoardDisplay[4][3]=BLACK_STONE;
	m_FlagForWhite=FALSE;
	m_PutNumber=0;
}

void ReReverse(Ando ando)
{
	int i=0;
	while(ando.position[i]!=-1)
	{
		m_BoardDisplay[ando.position[i]%MASU_NUM][ando.position[i]/MASU_NUM]*=-1;
		i++;
	}
	m_BoardDisplay[ando.x][ando.y]=BLANK;
	m_FlagForWhite=!m_FlagForWhite;
}

BOOL CanDropDown(int x,int y,int vect_x,int vect_y)
{
	int put_stone;
	if(m_FlagForWhite)put_stone=WHITE_STONE;
	else put_stone=BLACK_STONE;
	x+=vect_x;
	y+=vect_y;
	if(x<0||x>=MASU_NUM||y<0||y>=MASU_NUM)return(FALSE);
	if(m_BoardDisplay[x][y]==put_stone)return(FALSE);
	if(m_BoardDisplay[x][y]==BLANK)return(FALSE);
	x+=vect_x;
	y+=vect_y;
	while(x>=0&&x<MASU_NUM&&y>=0&&y<MASU_NUM)
	{
		if(m_BoardDisplay[x][y]==BLANK)return(FALSE);
		if(m_BoardDisplay[x][y]==put_stone)return(TRUE);
		x+=vect_x;
		y+=vect_y;
	}
	return(FALSE);
}

BOOL CanDropDown(int x,int y)
{
	if(x>=MASU_NUM||y>=MASU_NUM)
	{
		return FALSE;
	}
	if(m_BoardDisplay[x][y]!=BLANK)
	{
		return FALSE;
	}
	if(CanDropDown(x,y,1,0))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,0,1))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,-1,0))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,0,-1))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,1,1))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,-1,-1))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,1,-1))
	{
		return TRUE;
	}
	if(CanDropDown(x,y,-1,1))
	{
		return TRUE;
	}
	return FALSE;
}

int ValueBoardNumber()
{
	int x,y,value=0;
	for(x=0;x<MASU_NUM;x++)
	{
		for(y=0;y<MASU_NUM;y++)
		{
			value+=m_BoardDisplay[x][y];
		}
	}
	return value*-1;
}

int ValueBoardDropDownNum()
{
	int x,y,value=0;
	for(x=0;x<MASU_NUM;x++)
	{
		for(y=0;y<MASU_NUM;y++)
		{
			if(CanDropDown(x,y))value+=1;
		}
	}
	if(m_FlagForWhite==!m_FlagForPlayer)
	{
		return 3*value;
	}
	else
	{
		return -3*value;
	}
}

int ValueBoardPlace()
{
	int x,y,value=0;
	for(x=0;x<MASU_NUM;x++)
	{
		for(y=0;y<MASU_NUM;y++)
		{
			value+=m_BoardDisplay[x][y]*ValuePlace[x][y];
		}
	}
	return -value;
}

int ValueBoard()
{
	int value=0;
	if(m_PutNumber<=MIDDLE_NUM || m_PutNumber<=FINISH_NUM)
	{
		value+=ValueBoardPlace();
		value+=ValueBoardDropDownNum();
	}
	else
	{
		value+=ValueBoardNumber();
	}
	if(!m_FlagForPlayer)
	{
		return value;
	}
	else
	{
		return -value;
	}
}

void InitAndo(Ando*p_ando,int x,int y)
{
	p_ando->x=x;
	p_ando->y=y;
	p_ando->pointer=0;
	for(int i=0;i<30;i++)
	{
		p_ando->position[i]=-1;
	}
}

void Reverse(Ando*p_ando,int vect_x,int vect_y)
{
	int put_stone;
	int x=p_ando->x;
	int y=p_ando->y;
	int i=p_ando->pointer;
	if(m_FlagForWhite)
	{
		put_stone=WHITE_STONE;
	}
	else
	{
		put_stone=BLACK_STONE;
	}
	while(m_BoardDisplay[x+=vect_x][y+=vect_y]!=put_stone)
	{
		m_BoardDisplay[x][y]=put_stone;
		p_ando->position[i++]=x+y*MASU_NUM;
	}
	p_ando->position[p_ando->pointer=i]=-1;
}

void Reverse(Ando*p_ando)
{
	if(CanDropDown(p_ando->x,p_ando->y,1,0))
	{
		Reverse(p_ando,1,0);
	}
	if(CanDropDown(p_ando->x,p_ando->y,0,1))
	{
		Reverse(p_ando,0,1);
	}
	if(CanDropDown(p_ando->x,p_ando->y,-1,0))
	{
		Reverse(p_ando,-1,0);
	}
	if(CanDropDown(p_ando->x,p_ando->y,0,-1))
	{
		Reverse(p_ando,0,-1);
	}
	if(CanDropDown(p_ando->x,p_ando->y,1,1))
	{
		Reverse(p_ando,1,1);
	}
	if(CanDropDown(p_ando->x,p_ando->y,-1,-1))
	{
		Reverse(p_ando,-1,-1);
	}
	if(CanDropDown(p_ando->x,p_ando->y,1,-1))
	{
		Reverse(p_ando,1,-1);
	}
	if(CanDropDown(p_ando->x,p_ando->y,-1,1))
	{
		Reverse(p_ando,-1,1);
	}
}

void DropDownStone(int x,int y)
{
	int stone;
	if(m_FlagForWhite)
	{
		stone=WHITE_STONE;
	}
	else
	{
		stone=BLACK_STONE;
	}
	m_BoardDisplay[x][y]=stone;
	m_FlagForWhite=!m_FlagForWhite;
}

int Min_Max(BOOL Flag,int lv,BOOL Put,int alpha,int beta)
{
	int  temp,x,y,vest_x,vest_y;
	BOOL FlagForPut=FALSE;
	Ando ando;
	if(lv==0)
	{
		return ValueBoard();
	}
	if(Flag)
	{
		alpha=-9999;
	}
	else
	{
		beta=9999;
	}
	for(x=0;x<MASU_NUM;x++)
	{
		for(y=0;y<MASU_NUM;y++)
		{
			if(CanDropDown(x,y))
			{
				FlagForPut=TRUE;
				InitAndo(&ando,x,y);
				Reverse(&ando);
				DropDownStone(x,y);
				temp=Min_Max(!Flag,lv-1,TRUE,alpha,beta);
				ReReverse(ando);
				if(Flag)
				{
					if(temp>=alpha)
					{
						alpha=temp;
						vest_x=x;
						vest_y=y;
					}
					if(alpha>beta)
					{
						return alpha;
					}
				}
				else
				{
					if(temp<=beta)
					{
						beta=temp;
						vest_x=x;
						vest_y=y;
					}
					if(alpha>beta)
					{
						return beta;
					}
				}
			}
		}
	}
	if(FlagForPut)
	{
		if(lv==m_SearchLv)
		{
			return(vest_x+vest_y*MASU_NUM);
		}
		else if(Flag)
		{
			return alpha;
		}
		else
		{
			return beta;
		}
	}
	else if(!Put)
	{
		return ValueBoard();
	}
	else
	{
		m_FlagForWhite=!m_FlagForWhite;
		temp=Min_Max(!Flag,lv-1,FALSE,alpha,beta);
		m_FlagForWhite=!m_FlagForWhite;
		return temp;
	}
}

void ComputerAI()
{
	int x,y;
	Ando ando;
	if(m_PutNumber>=FINISH_NUM)
	{
		y=Min_Max(TRUE,m_SearchLv=12,TRUE,-9999,9999);
	}
	else
	{
		y=Min_Max(TRUE,m_SearchLv=SEARCH_LV,TRUE,-9999,9999);
	}
	if(0>y||y>=MASU_NUM*MASU_NUM)
	{
		m_FlagForWhite=!m_FlagForWhite;
		return;
	}
	x=y%MASU_NUM;
	y=y/MASU_NUM;
	InitAndo(&ando,x,y);
	Reverse(&ando);
	DropDownStone(x,y);
	m_PutNumber++;
	for(x=0;x<MASU_NUM*MASU_NUM;x++)
	{
		if(m_PutNumber==60)
		{
			break;
		}
		if(CanDropDown(x%MASU_NUM,x/MASU_NUM))
		{
			break;
		}
		if(x==MASU_NUM*MASU_NUM-1)
		{
			m_FlagForWhite=!m_FlagForWhite;
			ComputerAI();
		}
	}
}

DWORD WINAPI ThreadFunc(LPVOID)
{
	EnableMenuItem(GetMenu(hWnd),ID_START_BLACK,MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd),ID_START_WHITE,MF_GRAYED);
	DrawMenuBar(hWnd);
	ComputerAI();
	EnableMenuItem(GetMenu(hWnd),ID_START_BLACK,MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd),ID_START_WHITE,MF_ENABLED);
	DrawMenuBar(hWnd);
	PostMessage(hWnd,WM_ENDTHREAD,0,0);
	return(0);
}


BOOL InitGL(GLvoid)
{
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glClearColor(GetRValue(GetSysColor(COLOR_BTNFACE))/255.0f,GetGValue(GetSysColor(COLOR_BTNFACE))/255.0f,GetBValue(GetSysColor(COLOR_BTNFACE))/255.0f,0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	trans=-3.0f;
	return TRUE;
}

VOID DrawGLScene(GLvoid)
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0,1.0,-WINDOW_ASPECT,WINDOW_ASPECT,1.5,5.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0,0,trans);
	glRotatef(rot[0]+45,1.0f,0.0f,0.0f);
	glRotatef(rot[1],0.0f,1.0f,0.0f);
	glRotatef(90,1.0f,0.0f,0.0f);
	int x,y;
	for(x=0;x<MASU_NUM;x++)
	{
		for(y=0;y<MASU_NUM;y++)
		{
			if(m_BoardDisplay[x][y]==BLANK)
			{
				glColor3d(0.0,0.5,0.0);
			}
			else if(m_BoardDisplay[x][y]==BLACK_STONE)
			{
				glColor3d(0.0,0.0,0.0);
			}
			else
			{
				glColor3d(1.0,1.0,1.0);
			}
			glBegin(GL_POLYGON);
			glVertex2d(x*MASU_SIZE-1,-y*MASU_SIZE+1); // 第 1 象限
			glVertex2d((x+1)*MASU_SIZE-1,-y*MASU_SIZE+1); // 第 2 象限
			glVertex2d((x+1)*MASU_SIZE-1,-(y+1)*MASU_SIZE+1); // 第 3 象限
			glVertex2d(x*MASU_SIZE-1,-(y+1)*MASU_SIZE+1); // 第 4 象限
			glEnd();
		}
	}
	if(m_FlagInGame)
	{
		End();
	}
	glFlush();
}

static void rotate(int ox,int nx,int oy,int ny)
{
	int dx=ox-nx;
	int dy=ny-oy;
	rot[0]+=(dy*180.0f)/500.0f;
	rot[1]-=(dx*180.0f)/500.0f;
#define clamp(x) x=x>360.0f?x-360.0f:x<-360.0f?x+=360.0f:x
	clamp(rot[0]);
	clamp(rot[1]);
#undef clamp
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	static PIXELFORMATDESCRIPTOR pfd={sizeof(PIXELFORMATDESCRIPTOR),1,PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,32,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,PFD_MAIN_PLANE,0,0,0,0};
	GLuint PixelFormat;
	static BOOL bCapture=0;
	static int oldx,oldy,x,y;
	static HANDLE hThread;
	static DWORD d;
	switch(msg)
	{
	case WM_CREATE:
		if(!(hDC=GetDC(hWnd)))
		{
			return -1;
		}
		if(!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))
		{
			return -1;
		}
		if(!SetPixelFormat(hDC,PixelFormat,&pfd))
		{
			return -1;
		}
		if(!(hRC=wglCreateContext(hDC)))
		{
			return -1;
		}
		if(!wglMakeCurrent(hDC,hRC))
		{
			return -1;
		}
		if(!InitGL())
		{
			return -1;
		}
		m_FlagInGame=FALSE;
		break;
	case WM_ACTIVATE:
		active=!HIWORD(wParam);
		break;
	case WM_MOUSEWHEEL:
		trans+=(short)HIWORD(wParam)/120;
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		x=LOWORD(lParam);
		y=HIWORD(lParam);
		bCapture=1;
		{
			if(!m_FlagInGame)
			{
				return 0;
			}
			if(hThread)
			{
				return 0;
			}
			int x=LOWORD(lParam);
			int y=HIWORD(lParam);
			double modelview[16];
			glGetDoublev(GL_MODELVIEW_MATRIX,modelview);
			double projection[16];
			glGetDoublev(GL_PROJECTION_MATRIX,projection);
			int viewport[4];
			glGetIntegerv(GL_VIEWPORT,viewport);
			static float z;
			static double objX,objY,objZ;
			glReadPixels(x,WINDOW_HEIGHT-y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&z);
			gluUnProject(x,WINDOW_HEIGHT-y,z,modelview,projection,viewport,&objX,&objY,&objZ);
			if(objZ<-0.01)return 0;
			if(objZ>0.01)return 0;
			if(objX<-1.0)return 0;
			if(objX>1.0)return 0;
			if(objY<-1.0)return 0;
			if(objY>1.0)return 0;
			int nPosX = (int)((objX+1)/MASU_SIZE);
			int nPosY =	(int)((1-objY)/MASU_SIZE);	
			if(CanDropDown(nPosX,nPosY))
			{
				ReleaseCapture();
				bCapture=0;
				Ando ando;
				InitAndo(&ando,nPosX,nPosY);
				Reverse(&ando);
				DropDownStone(nPosX,nPosY);
				m_PutNumber++;
				if(m_FlagInGame)
				{
					hThread=CreateThread(
						0,
						0,
						ThreadFunc,
						0,
						0,
						&d);
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		bCapture=0;
		break;
	case WM_MOUSEMOVE:
		if(bCapture)
		{
			oldx=x;
			oldy=y;
			x=LOWORD(lParam);
			y=HIWORD(lParam);
			if(x&1<<15)x-=(1<<16);
			if(y&1<<15)y-=(1<<16);
			rotate(oldx,x,oldy,y);
		}
		break;
	case WM_DESTROY:
		if(hRC)
		{
			wglMakeCurrent(NULL,NULL);
			wglDeleteContext(hRC);
		}
		if(hDC)
		{
			ReleaseDC(hWnd,hDC);
		}
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case ID_START_BLACK:
			m_FlagInGame=TRUE;
			m_FlagForPlayer=FALSE;
			InitBoard();
			break;
		case ID_START_WHITE:
			m_FlagInGame=TRUE;
			m_FlagForPlayer=TRUE;
			InitBoard();
			hThread=CreateThread(
				NULL,
				0,
				ThreadFunc,
				0,
				0,
				&d);
			break;
		}
		break;
	case WM_ENDTHREAD:
		WaitForSingleObject(hThread,INFINITE);
		CloseHandle(hThread);
		hThread=0;
		break;
	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
	default:
		return DefWindowProc(hWnd,msg,wParam,lParam);
	}
	return 0;
}

EXTERN_C void __cdecl WinMainCRTStartup()
{
	MSG msg;
	HINSTANCE hInstance=GetModuleHandle(0);
	WNDCLASS wndclass={
		0,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		0,
		(LPCTSTR)IDR_MENU1,
		szClassName
	};
	RegisterClass(&wndclass);
	RECT rect={0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
	AdjustWindowRect(
		&rect,
		WS_OVERLAPPED|
		WS_CAPTION|
		WS_SYSMENU|
		WS_CLIPSIBLINGS|
		WS_CLIPCHILDREN,
		TRUE);
	hWnd=CreateWindow(
		szClassName,
		TEXT("3D Othello"),
		WS_OVERLAPPED|
		WS_CAPTION|
		WS_SYSMENU|
		WS_CLIPSIBLINGS|
		WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		rect.right-rect.left,
		rect.bottom-rect.top,
		0,
		0,
		hInstance,
		0);
	ShowWindow(hWnd,SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	BOOL done=FALSE;
	while(!done)
	{
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{
			if(msg.message==WM_QUIT)
			{
				done=TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if(active)
		{
			DrawGLScene();
			SwapBuffers(hDC);
		}
	}
	ExitProcess(0);
}

#if _DEBUG
void main(){}
#endif
