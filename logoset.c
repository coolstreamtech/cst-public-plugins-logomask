#include <string.h>
#include <time.h>
#include "logoset.h"
#include "io.h"
#include "gfx.h"
#include "text.h"

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

#define NCF_FILE "/var/tuxbox/config/neutrino.conf"
#define CFG_FILE "/var/tuxbox/config/logomask.conf"
unsigned char FONT[64]= "/share/fonts/pakenham.ttf";

#define CL_VERSION  "1.01"
#define MAX_MASK 16

//					TRANSP,	BLACK,	RED, 	GREEN, 	YELLOW,	BLUE, 	MAGENTA, TURQUOISE,
//					WHITE, 	GRAY, 	LRED,	LGREEN,	LYELLOW,LBLUE,	LMAGENTA,LTURQUOISE
unsigned char
			rd[]={	0x00,	0x00,	0x80,	0x00,	0x80,	0x00,	0x80,	0x00,
					0xFF,	0x80,	0xFF,	0x00,	0xFF,	0x00,	0xFF,	0x00
				 },
			gn[]={	0x00,	0x00,	0x00,	0x80,	0x80,	0x00,	0x00,	0x80,
					0xFF,	0x80,	0x00,	0xFF,	0xFF,	0x00,	0x00,	0xFF
				 },
			 bl[]={	0x00,	0x00,	0x00,	0x00,	0x00,	0x80,	0x80,	0x80,
					0xFF,	0x80,	0x00,	0x00,	0x00,	0xFF,	0xFF,	0xFF
			     },
			 tr[]={	0x00,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,
					0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF
				  };


unsigned char *lfb = 0, *lbb = 0;
char tstr[BUFSIZE];
gpixel lpix;


void TrimString(char *strg)
{
char *pt1=strg, *pt2=strg;

	while(*pt2 && *pt2<=' ')
	{
		++pt2;
	}
	if(pt1 != pt2)
	{
		do
		{
			*pt1=*pt2;
			++pt1;
			++pt2;
		}
		while(*pt2);
		*pt1=0;
	}
	while(strlen(strg) && strg[strlen(strg)-1]<=' ')
	{
		strg[strlen(strg)-1]=0;
	}
}

/******************************************************************************
 * logoset Main
 ******************************************************************************/

int main (int argc, char **argv)
{
	int i,j,found=0,m,mask=1,kmode=1,pmode=0, lc=-1, changed=0, todo=1, help=1, help_changed=0, move=0;
	unsigned char actchan[20]=""/*,channel[128]=""*/;
	FILE *fh,*fh2;
	char *cpt1,*cpt2;
	gpixel mp, mc[MAX_MASK], tp;
	int tsx=430, tsy=120, tdy=24, tsz=28, txw=500, tcol=TURQUOISE;
	int xp[MAX_MASK][8],yp[MAX_MASK][8],xw[MAX_MASK][8],yw[MAX_MASK][8],valid[MAX_MASK],cmc[MAX_MASK],xxp,xxw,yyp,yyw,nmsk=0,amsk=0;
	double xs=1.0, ys=1.0;
	time_t t1,t2;

		for(j=0; j<MAX_MASK; j++)
		{
			valid[j]=0;
			cmc[j]=BLACK;
			make_color(BLACK, &mc[j]);
			for(i=0; i<8; i++)
			{
				xp[j][i]=(1280-40)/2;
				xw[j][i]=40;
				yp[j][i]=(720-20)/2;
				yw[j][i]=20;
			}	
		}
		system("pzapit -var > /tmp/logomaskset.stat");
		if((fh=fopen("/tmp/logomaskset.stat","r"))!=NULL)
		{
			if(fgets(tstr,500,fh))
			{
				TrimString(tstr);
				if(strlen(tstr))
				{
					if(sscanf(tstr+strlen(tstr)-1,"%d",&pmode)!=1)
					{
						pmode=0;
					}
				}
			}
			fclose(fh);
		}
	
		system("touch /tmp/.logomask_kill");

		fb = open(FB_DEVICE, O_RDWR);

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("logomask <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("logomask <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("logomask <mapping of Framebuffer failed>\n");
			return -1;
		}

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("logomask <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("logomask <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("logomask <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_LookupFace(manager, FONT, &face)))
		{
			printf("logomask <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.face_id = FONT;
		desc.flags = FT_LOAD_MONOCHROME;


		InitRC();

	//init backbuffer

		if(!(lbb = malloc(fix_screeninfo.line_length*var_screeninfo.yres)))
		{
			printf("logomask <allocating of Backbuffer failed>\n");
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, 0, fix_screeninfo.line_length*var_screeninfo.yres);

		system("pzapit -gi > /tmp/logomask.chan");
		if((fh=fopen("/tmp/logomask.chan","r"))!=NULL)
		{
			if(fgets(tstr, BUFSIZE, fh))
			{
				TrimString(tstr);
				if((cpt1=strchr(tstr,' '))!=NULL)
					*cpt1=0;
			}
			fclose(fh);
			if(strlen(tstr))
			{
				strcpy(actchan,tstr);
			}

			if((fh=fopen(CFG_FILE,"r"))!=NULL)
			{
				found=0;
				while(fgets(tstr, BUFSIZE, fh) && !found)
				{
					TrimString(tstr);
					if(strlen(tstr))
					{
						if(strstr(tstr,actchan)!=NULL)
						{
							mask=1;
							nmsk=0;
							cpt2=strstr(tstr,",MC");
							if((cpt1=strchr(tstr,','))!=NULL)
							{
								while(cpt1)
								{
									valid[nmsk]=0;
									if(cpt2 && sscanf(cpt2+1,"MC%8X",&mp.lpixel)==1)
									{
										cpt2=strchr(cpt2+1,',');
									}
									else
									{
										make_color(BLACK, &mp);
									}
									for(i=0; i<8 && cpt1; i++)
									{
										cpt1++;
										if(sscanf(cpt1,"%d,%d,%d,%d",&xxp,&xxw,&yyp,&yyw)==4)
										{
											xp[nmsk][i]=xxp;
											xw[nmsk][i]=xxw;
											yp[nmsk][i]=yyp;
											yw[nmsk][i]=yyw;
											mc[nmsk].lpixel=mp.lpixel;
											found=1;
											valid[nmsk]=1;
										}
										for(j=0; j<4 && cpt1; j++)
										{
											cpt1=strchr(cpt1+1,',');
										}
									}
									if(valid[nmsk])
									{
										nmsk++;
									}
								}
							}
						}
					}
				}
				fclose(fh);
			}
		}

		if(!nmsk)
		{
			nmsk=1;
			valid[0]=1;
		}
		mask=nmsk;
		for(m=0; m<MAX_MASK; m++)
		{
			if(valid[m])
			{
				xxp=xp[m][pmode];
				xxw=xw[m][pmode];				
				yyp=yp[m][pmode];
				yyw=yw[m][pmode];
				tp.lpixel=mc[m].lpixel;
				RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, &tp);
				if(m==amsk)
					RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, GRID, make_color(LBLUE,&tp));
				for(i=0;i<=yyw;i++)
				{
					j=(yyp+i)*fix_screeninfo.line_length+(xxp<<2);
					if((j+(xxw<<2))<fix_screeninfo.line_length*var_screeninfo.yres)
					{
						memcpy(lfb+j, lbb+j, xxw<<2);
					}
				}
			}
		}
		time(&t1);
		while((rc!=KEY_EXIT) && (rc!=KEY_OK))
		{
			rc=GetRCCode();
			if((rc!=-1) && (rc!=KEY_EXIT) && (rc!=KEY_OK))
			{
				time(&t1);
				move=0;
				xxp=xp[amsk][pmode];
				xxw=xw[amsk][pmode];
				yyp=yp[amsk][pmode];
				yyw=yw[amsk][pmode];
				lpix.lpixel=mc[amsk].lpixel;
				switch(rc)
				{
					case KEY_LEFT:
					if(lc==KEY_LEFT)
					{
						xs+=0.3;
					}
					else
					{
						xs=1.0;
					}
					if(kmode)
					{
						if(xxp>0)
						{
							changed=1;
							xxp-=xs;
						}
					}
					else
					{
						if(xxw>6)
						{
							changed=1;
							xxw-=xs;
						}
					}
					move=1;
					break;
				
					case KEY_RIGHT:
					if((xxp+xxw)<(fix_screeninfo.line_length-1))
					{
						changed=1;
						if(lc==KEY_RIGHT)
						{
							xs+=0.3;
						}
						else
						{
							xs=1.0;
						}
						if(kmode)
						{
							xxp+=xs;
						}
						else
						{
							xxw+=xs;
						}
					}
					move=1;
					break;
				
					case KEY_UP:
					if(lc==KEY_UP)
					{
						ys+=0.2;
					}
					else
					{
						ys=1.0;
					}
					if(kmode)
					{
						if(yyp>0)
						{
							changed=1;
							yyp-=ys;
						}
					}
					else
					{
						if(yyw>6)
						{
							changed=1;
							yyw-=ys;
						}
					}
					move=1;
					break;
				
					case KEY_DOWN:
					if((yyp+yyw)<(var_screeninfo.yres-1))
					{
						changed=1;
						if(lc==KEY_DOWN)
						{
							ys+=0.2;
						}
						else
						{
							ys=1.0;
						}
						if(kmode)
						{
							yyp+=ys;
						}
						else
						{
							yyw+=ys;
						}
					}
					move=1;
					break;
				
					case KEY_RED:
						changed=1;
						RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, make_color(TRANSP,&tp));
						for(i=0;i<=yyw;i++)
						{
							j=(yyp+i)*fix_screeninfo.line_length+(xxp<<2);
							if(((j+(xxw<<2)))<fix_screeninfo.line_length*var_screeninfo.yres)
							{
								memcpy(lfb+j, lbb+j, xxw<<2);
							}
						}
						valid[amsk]=0;
						nmsk--;
						kmode=1;
						if(nmsk)
						{
							todo=2;
							amsk=-1;
							for(m=0; m<MAX_MASK && amsk<0; m++)
							{
								if(valid[m])
								{
									amsk=m;
									xxp=xp[amsk][pmode];
									xxw=xw[amsk][pmode];
									yyp=yp[amsk][pmode];
									yyw=yw[amsk][pmode];
									lpix.lpixel=mc[amsk].lpixel;
								}
							}	
						}
						else
						{
							todo=mask=0;
						}
					break;
				
					case KEY_GREEN:
						if(nmsk<MAX_MASK)
						{
							todo=2;
							changed=1;
							kmode=1;
							amsk=-1;
							for(m=0; amsk<0 && m<MAX_MASK; m++)
							{
								if(!valid[m])
								{
									amsk=m;
									valid[amsk]=1;
									nmsk++;
									cmc[amsk]=BLACK;
									make_color(BLACK, &mc[amsk]);
									for(i=0; i<8; i++)
									{
										xp[amsk][i]=(1280-40)/2;
										xw[amsk][i]=40;
										yp[amsk][i]=(720-20)/2;
										yw[amsk][i]=20;
									}
									xxp=xp[amsk][pmode];
									xxw=xw[amsk][pmode];
									yyp=yp[amsk][pmode];
									yyw=yw[amsk][pmode];
									lpix.lpixel=mc[amsk].lpixel;
								}
							}
						}	
					break;
					
					case KEY_PAGEUP:
						if(nmsk>1)
						{
							m=amsk+1;
							if(m>=MAX_MASK)
							{
								m=0;
							}
							while(!valid[m])
							{
								if(++m>=MAX_MASK)
								{
									m=0;
								}
							}
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, &lpix);
							amsk=m;
							xxp=xp[amsk][pmode];
							xxw=xw[amsk][pmode];
							yyp=yp[amsk][pmode];
							yyw=yw[amsk][pmode];
							lpix.lpixel=mc[amsk].lpixel;
						}
					break;
				
					case KEY_PAGEDOWN:
						if(nmsk>1)
						{
							m=amsk-1;
							if(m<0)
							{
								m=MAX_MASK-1;
							}
							while(!valid[m])
							{
								if(--m<0)
								{
									m=MAX_MASK;
								}
							}
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, &lpix);
							amsk=m;
							xxp=xp[amsk][pmode];
							xxw=xw[amsk][pmode];
							yyp=yp[amsk][pmode];
							yyw=yw[amsk][pmode];
							lpix.lpixel=mc[amsk].lpixel;
						}
					break;

					case KEY_YELLOW:
						kmode=0;
					break;
				
					case KEY_BLUE:
						kmode=1;
					break;
					
					case KEY_1:
						if(nmsk)
						{
							if(mc[amsk].cpixel.rd < 0xF0)
								mc[amsk].cpixel.rd+=0x10;
							else
								mc[amsk].cpixel.rd=0xFF;
							changed=1;
						}
					break;

					case KEY_4:
						if(nmsk)
						{
							mc[amsk].cpixel.rd=0x80;
							changed=1;
						}
					break;

					case KEY_7:
						if(nmsk)
						{
							if(mc[amsk].cpixel.rd > 0x0F)
								mc[amsk].cpixel.rd-=0x10;
							else
								mc[amsk].cpixel.rd=0x00;
							changed=1;
						}
					break;

					case KEY_2:
						if(nmsk)
						{
							if(mc[amsk].cpixel.gn < 0xF0)
								mc[amsk].cpixel.gn+=0x10;
							else
								mc[amsk].cpixel.gn=0xFF;
							changed=1;
						}
					break;

					case KEY_5:
						if(nmsk)
						{
							mc[amsk].cpixel.gn=0x80;
							changed=1;
						}
					break;

					case KEY_8:
						if(nmsk)
						{
							if(mc[amsk].cpixel.gn > 0x0F)
								mc[amsk].cpixel.gn-=0x10;
							else
								mc[amsk].cpixel.gn=0x00;
							changed=1;
						}
					break;

					case KEY_3:
						if(nmsk)
						{
							if(mc[amsk].cpixel.bl < 0xF0)
								mc[amsk].cpixel.bl+=0x10;
							else
								mc[amsk].cpixel.bl=0xFF;
							changed=1;
						}
					break;

					case KEY_6:
						if(nmsk)
						{
							mc[amsk].cpixel.bl=0x80;
							changed=1;
						}
					break;

					case KEY_9:
						if(nmsk)
						{
							if(mc[amsk].cpixel.bl > 0x0F)
								mc[amsk].cpixel.bl-=0x10;
							else
								mc[amsk].cpixel.bl=0x00;
							changed=1;
						}
					break;

					case KEY_VOLUMEDOWN:
						if(nmsk)
						{
							if(mc[amsk].cpixel.tr < 0xF8)
								mc[amsk].cpixel.tr+=0x08;
							else
								mc[amsk].cpixel.tr=0xFF;
							changed=1;
						}
					break;

					case KEY_VOLUMEUP:
						if(nmsk)
						{
							if(mc[amsk].cpixel.tr > 0x08)
								mc[amsk].cpixel.tr-=0x08;
							else
								mc[amsk].cpixel.tr=0x00;
							changed=1;
						}
					break;

					case KEY_MUTE:
						if(nmsk)
						{
							if(++cmc[amsk]>LTURQUOISE)
								cmc[amsk]=BLACK;
							make_color(cmc[amsk], &mc[amsk]);
							changed=1;
						}
					break;

					case KEY_HELP:
						help_changed=1;
					break;
				}
				lc=rc;
				lpix.lpixel=mc[amsk].lpixel;
				if(mask || todo==2)
				{
					RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], FILL, make_color(TRANSP, &tp));
					for(i=0;i<=yw[amsk][pmode];i++)
					{
						j=(yp[amsk][pmode]+i)*fix_screeninfo.line_length+(xp[amsk][pmode]<<2);
						if((j+(xw[amsk][pmode]<<2))<fix_screeninfo.line_length*var_screeninfo.yres)
						{
							memcpy(lfb+j, lbb+j, (xw[amsk][pmode]+1)<<2);
						}
					}
					xp[amsk][pmode]=xxp;
					xw[amsk][pmode]=xxw;
					yp[amsk][pmode]=yyp;
					yw[amsk][pmode]=yyw;
					for(m=0; mask && m<MAX_MASK; m++)
					{
						if(valid[m])
						{
							xxp=xp[m][pmode];
							xxw=xw[m][pmode];
							yyp=yp[m][pmode];
							yyw=yw[m][pmode];
							tp.lpixel=mc[m].lpixel;
							RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, FILL, ((m==amsk) && move)?make_color(TRANSP, &tp):&tp);
							if(m==amsk)
								RenderBox(xxp, yyp, xxp+xxw, yyp+yyw, GRID, make_color((kmode)?LBLUE:LYELLOW,&tp));
							for(i=0;i<=yyw;i++)
							{
								j=(yyp+i)*fix_screeninfo.line_length+(xxp<<2);
								if((j+(xxw<<2))<fix_screeninfo.line_length*var_screeninfo.yres)
								{
									memcpy(lfb+j, lbb+j, (xxw+1)<<2);
								}
							}
						}
					}
				}
			}
			time(&t2);
			if((t2-t1)>1)
			{
				xs=1.0;
				ys=1.0;
				tsy=120;
				if(move)
				{
					RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], FILL, &mc[amsk]);
					RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], GRID, make_color((kmode)?LBLUE:LYELLOW,&tp));
				}
				move=0;
				if(help_changed)
				{
					help^=1;
				}
				if(help)
				{
					RenderBox(tsx,tsy,tsx+txw,tsy+21*tdy,FILL,make_color(TRANSP, &tp));
					if(nmsk)
						RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], GRID, make_color((kmode)?LBLUE:LYELLOW, &tp));
					RenderString("Maskensteuerung", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Blau  :  Umschalten auf Positionseinstellung", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Gelb  :  Umschalten auf Gr��eneinstellung", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Gr�n  :  Maske hinzuf�gen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Rot    :  Maske l�schen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("PgUp :  n�chste Maske ausw�hlen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("PgDn :  vorherige Maske ausw�hlen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Maskenfarbe", tsx, tsy+=(2*tdy), txw, LEFT, tsz, tcol);
					RenderString("Mute  :  Maskenfarbe aus Vorgabe ausw�hlen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("1,4,7   :  Farbton Rot erh�hen, auf Mitte setzen, verringern", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("2,5,8  :  Farbton Gr�n erh�hen, auf Mitte setzen, verringern", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("3,6,9  :  Farbton Blau erh�hen, auf Mitte setzen, verringern", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Vol +  :  Transparenz erh�hen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Vol -  :  Transparenz verringern", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Allgemein", tsx, tsy+=(2*tdy), txw, LEFT, tsz, tcol);
					RenderString("?        :  Hilfetext ein/ausschalten", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("Exit    :  Abbrechen", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
					RenderString("OK     :  Speichern und Beenden", tsx, tsy+=tdy, txw, LEFT, tsz, tcol);
				}
				else
				{
					if(help_changed)
					{
						RenderBox(tsx, tsy, tsx+txw, tsy+21*tdy, FILL, make_color(TRANSP, &tp));
						if(nmsk)
							RenderBox(xp[amsk][pmode], yp[amsk][pmode], xp[amsk][pmode]+xw[amsk][pmode], yp[amsk][pmode]+yw[amsk][pmode], GRID, make_color((kmode)?LBLUE:LYELLOW, &tp));
					}
				}
				help_changed=0;
				memcpy(lfb, lbb, fix_screeninfo.line_length*var_screeninfo.yres);
			}
		}
		if(rc==KEY_EXIT)
		{
			changed=0;
			todo=0;
		}
		if(rc==KEY_OK && changed)
		{
			if((fh2=fopen("/tmp/logomask.conf","w"))!=NULL)
			{
				fh=fopen(CFG_FILE,"r");
				while(fh && fgets(tstr, BUFSIZE, fh))
				{
					TrimString(tstr);
					if(strlen(tstr))
					{
						if(strstr(tstr,actchan)==NULL)
						{
							fprintf(fh2,"%s\n",tstr);
						}
					}
				}
				if(fh)
				{
					fclose(fh);
				}
				if(todo)
				{
					fprintf(fh2,"%s",actchan);
					for(j=0; j<MAX_MASK; j++)
					{
						if(valid[j])
						{
							for(i=0; i<8; i++)
							{
								fprintf(fh2,",%d,%d,%d,%d",xp[j][i],xw[j][i],yp[j][i],yw[j][i]);
							}
						}
					}
					for(j=0; j<MAX_MASK; j++)
					{
						if(valid[j])
						{
							fprintf(fh2,",MC%08X",mc[j].lpixel);
						}
					}
					fprintf(fh2,",\n");
				}
				fclose(fh2);
				remove(CFG_FILE);
				system("mv /tmp/logomask.conf /var/tuxbox/config/logomask.conf");
			}
		}		
		free(lbb);
		munmap(lfb, fix_screeninfo.smem_len);

		close(fb);
		CloseRC();
		remove("/tmp/.logomask_kill");
		remove("/tmp/logomaskset.*");
		system("logomask &");
		return 0;
}

