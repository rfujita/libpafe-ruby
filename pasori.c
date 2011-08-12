/* $Id: pasori.c,v 1.19 2009-10-31 08:58:23 hito Exp $ */
#include <ruby.h>
#include <libpafe/libpafe.h>

static VALUE ePasoriError, cPasori, cFelica;
static ID ID_close, ID_length, ID_unpack, ID_bracket, ID_to_i;

static VALUE rb_pasori_new(int argc, VALUE *argv, VALUE klass);
static VALUE rb_pasori_open(int argc, VALUE *argv, VALUE klass);
static VALUE rb_pasori_close(VALUE obj_pasori);
static VALUE rb_pasori_read(VALUE obj_pasori);
static VALUE rb_pasori_write(VALUE obj_pasori, VALUE str);
static VALUE rb_pasori_send(VALUE obj_pasori, VALUE str);
static VALUE rb_pasori_set_timeout(VALUE obj_pasori, VALUE val);
static VALUE rb_pasori_recv(VALUE obj_pasori);
static VALUE rb_pasori_felica_polling(int argc, VALUE *argv, VALUE obj_pasori);
static VALUE rb_pasori_type(VALUE obj_pasori);

static VALUE c_pasori_close(VALUE obj_pasori);
static VALUE c_pasori_open(void);

static void  free_cpasori(pasori *cpasori);
static pasori *get_cpasori(VALUE obj);
static VALUE pasori_call_close(VALUE obj_pasori);


static VALUE rb_felica_new(int argc, VALUE *argv, VALUE klass);
static VALUE rb_felica_polling(int argc, VALUE *argv, VALUE klass);
static VALUE rb_felica_close(VALUE self);
static VALUE rb_felica_read(int argc, VALUE *argv, VALUE self);
static VALUE rb_felica_get_idm(VALUE self);
static VALUE rb_felica_request_service(VALUE self, VALUE ary);
static VALUE rb_felica_request_responce(VALUE self);
static VALUE rb_felica_request_system(VALUE self);
static VALUE rb_felica_search_sevice(VALUE self);
static VALUE rb_felica_get_pmm(VALUE self);
static VALUE rb_felica_get_service(VALUE self);
static VALUE rb_felica_get_area(VALUE self);

static VALUE c_felica_read(VALUE self, VALUE service, VALUE mode, VALUE addr);
static VALUE c_felica_get_area(VALUE self, char *type);
static VALUE c_felica_polling(VALUE obj_pasori, int system, int rfu, int timeslot);

static void free_cfelica(felica *cfelica);
static felica *get_cfelica(VALUE obj);

static VALUE rb_felica_area_new(VALUE klass, VALUE attr, VALUE code, VALUE bin);
static VALUE rb_felica_area_get_attr(VALUE self);
static VALUE rb_felica_area_get_code(VALUE self);
static VALUE rb_felica_area_get_bin(VALUE self);
static VALUE rb_felica_area_is_protected(VALUE self);

void Init_pasori(void);
void Init_felica(void);
void Init_felica_area(void);

/******************************************************************************/
/* Pasori object */

static void
free_cpasori(pasori *cpasori)
{
  if (cpasori) {
    pasori_close(cpasori);
  }
}

static pasori *
get_cpasori(VALUE obj)
{
  pasori *p;

  Data_Get_Struct(obj, pasori, p);
  if (p == NULL) {
    rb_raise(ePasoriError, "%s", "PaSoRi is already closed.");
  }
  return p;
}


static VALUE
c_pasori_open(void)
{
  pasori *p;
  VALUE obj;
  int r;

  p = pasori_open();
  if (p == NULL) {
    rb_raise(ePasoriError, "%s", "Can't open PaSoRi.");
  }

  r = pasori_init(p);
  if (r) {
    pasori_close(p);
    rb_raise(ePasoriError, "%s", "Can't init PaSoRi.");
  }

  obj = Data_Wrap_Struct(cPasori, NULL, free_cpasori, p);

  return obj;
}

static VALUE
rb_pasori_new(int argc, VALUE *argv, VALUE klass)
{
  if (rb_block_given_p()) {
    const char *cname = rb_class2name(klass);

    rb_warn("%s::new() does not take block; use %s::open() instead",
	    cname, cname);
  }
  return c_pasori_open();
}

static VALUE
rb_pasori_open(int argc, VALUE *argv, VALUE klass)
{
  VALUE obj_pasori = c_pasori_open();

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, obj_pasori, c_pasori_close, obj_pasori);
  }

  return obj_pasori;
}

static VALUE
rb_pasori_close(VALUE obj_pasori)
{
  pasori *p;

  p = get_cpasori(obj_pasori);
  free_cpasori(p);
  RDATA(obj_pasori)->data = NULL;

  return Qnil;
}

static VALUE
c_pasori_close(VALUE obj_pasori)
{
  return rb_rescue(pasori_call_close, obj_pasori, 0, 0);
}

static VALUE
pasori_call_close(VALUE obj_pasori)
{
  return rb_funcall(obj_pasori, ID_close, 0);
}

static VALUE 
rb_pasori_read(VALUE obj_pasori)
{
  pasori *p;
  int len;
  uint8 b[255];

  p = get_cpasori(obj_pasori);

  len = sizeof(b);
  if (pasori_read(p, b, &len))
    rb_raise(ePasoriError, "%s", "Can't read PaSoRi.");

  return rb_str_new((char *)b, len);
}

static VALUE 
rb_pasori_write(VALUE obj_pasori, VALUE str)
{
  pasori *p;
  int r, l;
  char *data;
  VALUE len;

  StringValue(str);
  len = rb_funcall(str, ID_length, 0);
  l = NUM2INT(len);
  data = StringValuePtr(str);

  p = get_cpasori(obj_pasori);

  r = pasori_write(p, (uint8 *)data, &l);
  if (r)
    rb_raise(ePasoriError, "%s", "Can't write PaSoRi.");

  return INT2FIX(l);
}

static VALUE 
rb_pasori_send(VALUE obj_pasori, VALUE str)
{
  pasori *p;
  int r, l;
  VALUE len;
  char *data;
  
  StringValue(str);
  len = rb_funcall(str, ID_length, 0);
  l = NUM2INT(len);
  data = StringValuePtr(str);

  p = get_cpasori(obj_pasori);

  r = pasori_send(p, (uint8 *)data, &l);
  if (r)
    rb_raise(ePasoriError, "%s", "Can't read PaSoRi.");

  return INT2FIX(l);
}

static VALUE 
rb_pasori_set_timeout(VALUE obj_pasori, VALUE val)
{
  pasori *p;
  int timeout;

  Check_Type(val, T_FIXNUM);

  p = get_cpasori(obj_pasori);

  timeout= FIX2INT(val);
  pasori_set_timeout(p, timeout);

  return obj_pasori;
}

static VALUE 
rb_pasori_recv(VALUE obj_pasori)
{
  pasori *p;
  int len, r;
  uint8 b[255];

  p = get_cpasori(obj_pasori);

  len = sizeof(b);
  
  r = pasori_recv(p, b, &len);
  if (r)
    rb_raise(ePasoriError, "%s", "Can't read PaSoRi.");

  return rb_str_new((char *)b, len);
}

static VALUE
rb_pasori_felica_polling(int argc, VALUE *argv, VALUE obj_pasori)
{
  int i;
  VALUE system, rfu, timeslot, cfelica;

  i = rb_scan_args(argc, argv, "03", &system, &rfu, &timeslot);
  switch (i) {
  case 0:
    system = INT2NUM(FELICA_POLLING_ANY);
  case 1:
    Check_Type(system, T_FIXNUM);
    rfu = INT2NUM(0);
  case 2:
    Check_Type(rfu, T_FIXNUM);
    timeslot = INT2NUM(0);
  }

  cfelica = c_felica_polling(obj_pasori, NUM2INT(system), NUM2INT(rfu), NUM2INT(timeslot));

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, cfelica, rb_felica_close, cfelica);
  }

  return cfelica;
}

static VALUE 
rb_pasori_type(VALUE obj_pasori)
{
  pasori *p;

  p = get_cpasori(obj_pasori);

  return INT2FIX(pasori_type(p));
}

void 
Init_pasori(void)
{
  cPasori = rb_define_class("Pasori", rb_cObject);

  rb_define_singleton_method(cPasori, "open", rb_pasori_open, -1);
  rb_define_singleton_method(cPasori, "new", rb_pasori_new, -1);

  rb_define_method(cPasori, "close", rb_pasori_close, 0);
  rb_define_method(cPasori, "send", rb_pasori_send, 1);
  rb_define_method(cPasori, "recv", rb_pasori_recv, 0);
  rb_define_method(cPasori, "write", rb_pasori_write, 1);
  rb_define_method(cPasori, "read", rb_pasori_read, 0);
  rb_define_method(cPasori, "type", rb_pasori_type, 0);
  rb_define_method(cPasori, "set_timeout", rb_pasori_set_timeout, 1);
  rb_define_method(cPasori, "felica_polling", rb_pasori_felica_polling, -1);

  ePasoriError = rb_define_class("PasoriError", rb_eStandardError);

  rb_define_const(cPasori, "TYPE_S310", INT2NUM(PASORI_TYPE_S310));
  rb_define_const(cPasori, "TYPE_S320", INT2NUM(PASORI_TYPE_S320));
  rb_define_const(cPasori, "TYPE_S330", INT2NUM(PASORI_TYPE_S330));

  Init_felica();
  Init_felica_area();

  ID_close = rb_intern("close");
  ID_length = rb_intern("length");
}

/******************************************************************************/
/* Felica object */

static void
free_cfelica(felica *f)
{
  if (f) {
    free(f);
  }
}

static felica *
get_cfelica(VALUE obj)
{
  felica *f;

  Data_Get_Struct(obj, felica, f);
  if (f == NULL) {
    rb_raise(ePasoriError, "%s", "FeliCa is already closed.");
  }
  return f;
}

static VALUE
rb_felica_new(int argc, VALUE *argv, VALUE klass)
{
  if (rb_block_given_p()) {
    const char *cname = rb_class2name(klass);

    rb_warn("%s::new() does not take block; use %s::open() instead",
	    cname, cname);
  }
  return rb_felica_polling(argc, argv, klass);
}

static VALUE
rb_felica_polling(int argc, VALUE *argv, VALUE klass)
{
  VALUE obj_pasori, obj_felica, system, rfu, timeslot;
  int i;

  i = rb_scan_args(argc, argv, "22", &obj_pasori, &system, &rfu, &timeslot);

  switch (i) {
  case 2:
    rfu = INT2NUM(FELICA_POLLING_ANY);
  case 3:
    timeslot = INT2NUM(0);
  }

  obj_felica = c_felica_polling(obj_pasori, NUM2INT(system), NUM2INT(rfu), NUM2INT(timeslot));

  if (rb_block_given_p()) {
    return rb_ensure(rb_yield, obj_felica, rb_felica_close, obj_felica);
  }

  return obj_felica;
}

static VALUE
rb_felica_close(VALUE self)
{
  felica *f;

  rb_iv_set(self, "pasori", Qnil);
  f = get_cfelica(self);
  free(f);
  RDATA(self)->data = NULL;

  return Qnil;
}

static VALUE
c_felica_polling(VALUE obj_pasori, int system, int rfu, int timeslot)
{
  pasori *p;
  felica *f;
  VALUE obj;

  p = get_cpasori(obj_pasori);

  f = felica_polling(p, system, rfu, timeslot);
  if (f == NULL) {
    rb_raise(ePasoriError, "%s", "Can't open Felica.");
  }

  obj = Data_Wrap_Struct(cFelica, NULL, free_cfelica, f);
  rb_iv_set(obj, "pasori", obj_pasori);

  return obj;
}

static VALUE
rb_felica_read(int argc, VALUE *argv, VALUE self)
{
  VALUE service, mode, addr;
  int i;

  i = rb_scan_args(argc, argv, "21", &service, &addr, &mode);

  if (i == 2) {
    mode = INT2NUM(0);
  }

  return c_felica_read(self, service, mode, addr);
}

static VALUE
rb_felica_read_each(int argc, VALUE *argv, VALUE self)
{
  VALUE service, mode, r;
  int i;

  i = rb_scan_args(argc, argv, "11", &service, &mode);

  if (i == 1) {
    mode = INT2NUM(0);
  }

  i = 0;
  while (1) {
    r = c_felica_read(self, service, mode, INT2NUM(i));
    if (r == Qnil)
      break;
    rb_yield(r);
    i++;
  }
  return self;
}

static VALUE
c_felica_read(VALUE self, VALUE service, VALUE mode, VALUE addr)
{
  char data[FELICA_BLOCK_LENGTH];
  felica *f;
  VALUE s;
  int r;

  f = get_cfelica(self);
  s = rb_funcall(service, ID_to_i, 0);
  r = felica_read_single(f, NUM2INT(s), NUM2INT(mode), NUM2INT(addr), (uint8 *)data);

  if (r) {
    return Qnil;
  }

  return rb_str_new(data, sizeof(data));
}

static VALUE
rb_felica_get_idm(VALUE self)
{
  char idm[FELICA_IDM_LENGTH];
  felica *f;

  f = get_cfelica(self);
  felica_get_idm(f, (uint8 *)idm);

  return rb_str_new(idm, FELICA_IDM_LENGTH);
}

static VALUE
rb_felica_request_service(VALUE self, VALUE ary)
{
  felica *f;
  uint16 list[FELICA_AREA_NUM_MAX], data[FELICA_AREA_NUM_MAX];
  int i, r, n;
  VALUE r_ary;

  n = NUM2INT(rb_funcall(ary, ID_length, 0));
  if (n > FELICA_AREA_NUM_MAX) {
    rb_raise(ePasoriError, "%s", "Too many area list.");
  }

  for (i = 0; i < n; i++) {
    //    list[i] = NUM2INT(rb_funcall(rb_funcall(ary, ID_bracket, 1, INT2FIX(i)), ID_to_i, 0));
    list[i] = NUM2INT(rb_funcall(rb_ary_entry(ary, INT2FIX(i)), ID_to_i, 0));
  }

  f = get_cfelica(self);
  r = felica_request_service(f, &n, list, data);
  if (r) {
    rb_raise(ePasoriError, "%s (%d)", "Can't read FeliCa.", r);
  }

  r_ary = rb_ary_new();
  for (i = 0; i < n; i++) {
    rb_ary_push(r_ary, INT2NUM(data[i]));
  }

  return r_ary;
}

static VALUE
rb_felica_request_responce(VALUE self)
{
  felica *f;
  int r;
  uint8 m;

  f = get_cfelica(self);
  r = felica_request_response(f, &m);
  if (r) {
    rb_raise(ePasoriError, "%s", "Can't read FeliCa.");
  }

  return INT2NUM(m);
}

static VALUE
rb_felica_request_system(VALUE self)
{
  felica *f;
  uint16 data[256];
  int num = sizeof(data)/sizeof(*data);
  VALUE s;

  f = get_cfelica(self);
  felica_request_system(f, &num, data);

  s = rb_str_new((char *)data, num * sizeof(*data));
  return rb_funcall(s, ID_unpack, 1, rb_str_new("s*", 2));
}

static VALUE
rb_felica_search_sevice(VALUE self)
{
  felica *f;
  VALUE attr, service, area, klass;
  int r, i;

  f = get_cfelica(self);
  r = felica_search_service(f);
  if (r) {
    rb_raise(ePasoriError, "%s", "Can't read FeliCa.");
  }

  area = rb_ary_new();
  service = rb_ary_new();

  klass = rb_path2class("FelicaArea");
  for (i = 0; i < f->area_num; i++) {
    attr = rb_felica_area_new(klass,
			      INT2NUM(f->area[i].attr),
			      INT2NUM(f->area[i].code),
			      INT2NUM(f->area[i].bin));
    rb_ary_push(area, attr);
  }

  for (i = 0; i < f->service_num; i++) {
    attr = rb_felica_area_new(klass,
			      INT2NUM(f->service[i].attr),
			      INT2NUM(f->service[i].code),
			      INT2NUM(f->service[i].bin)
			      );
    rb_ary_push(service, attr);
  }

  rb_iv_set(self, "area", area);
  rb_iv_set(self, "service", service);

  return self;
}

static VALUE
rb_felica_get_pmm(VALUE self)
{
  char pmm[FELICA_PMM_LENGTH];
  felica *f;

  f = get_cfelica(self);
  felica_get_pmm(f, (uint8 *)pmm);
  return rb_str_new(pmm, FELICA_PMM_LENGTH);
}


static VALUE
c_felica_get_area(VALUE self, char *type)
{
  VALUE attr;

  attr = rb_iv_get(self, type);

  if (TYPE(attr) == T_NIL) {
    rb_felica_search_sevice(self);
    attr = rb_iv_get(self, type);
  }

  return attr;
}

static VALUE
rb_felica_get_service(VALUE self)
{
  return c_felica_get_area(self, "service");
}

static VALUE
rb_felica_get_area(VALUE self)
{
  return c_felica_get_area(self, "area");
}


void 
Init_felica(void) {
  cFelica = rb_define_class("Felica", rb_cObject);

  rb_define_singleton_method(cFelica, "new", rb_felica_new, -1);
  rb_define_singleton_method(cFelica, "polling", rb_felica_polling, -1);

  rb_define_method(cFelica, "close", rb_felica_close, 0);
  rb_define_method(cFelica, "idm", rb_felica_get_idm, 0);
  rb_define_method(cFelica, "pmm", rb_felica_get_pmm, 0);
  rb_define_method(cFelica, "service", rb_felica_get_service, 0);
  rb_define_method(cFelica, "area", rb_felica_get_area, 0);
  rb_define_method(cFelica, "read", rb_felica_read, -1);
  rb_define_method(cFelica, "foreach", rb_felica_read_each, -1);
  rb_define_method(cFelica, "request_service", rb_felica_request_service, 1);
  rb_define_method(cFelica, "request_response", rb_felica_request_responce, 0);
  rb_define_method(cFelica, "request_system", rb_felica_request_system, 0);
  rb_define_method(cFelica, "search_service", rb_felica_search_sevice, 0);

  rb_define_const(cFelica, "POLLING_ANY", INT2NUM(FELICA_POLLING_ANY));
  rb_define_const(cFelica, "POLLING_SUICA", INT2NUM(FELICA_POLLING_SUICA));
  rb_define_const(cFelica, "POLLING_EDY", INT2NUM(FELICA_POLLING_EDY));

  rb_define_const(cFelica, "SERVICE_IRUCA_HISTORY", INT2NUM(FELICA_SERVICE_IRUCA_HISTORY));
  rb_define_const(cFelica, "SERVICE_SUICA_HISTORY", INT2NUM(FELICA_SERVICE_SUICA_HISTORY));
  rb_define_const(cFelica, "SERVICE_SUICA_IN_OUT", INT2NUM(FELICA_SERVICE_SUICA_IN_OUT));
  rb_define_const(cFelica, "SERVICE_EDY_HISTORY", INT2NUM(FELICA_SERVICE_EDY_HISTORY));
  rb_define_const(cFelica, "SERVICE_EDY_NUMBER", INT2NUM(FELICA_SERVICE_EDY_NUMBER));


  ID_unpack = rb_intern("unpack");
  ID_bracket = rb_intern("[]");
  ID_to_i = rb_intern("to_i");
}

/******************************************************************************/
/* FelicaArea object */



static VALUE
rb_felica_area_new(VALUE klass, VALUE attr, VALUE code, VALUE bin)
{
  VALUE area;

  Check_Type(attr, T_FIXNUM);
  Check_Type(code, T_FIXNUM);
  
  if (TYPE(bin) != T_FIXNUM && TYPE(bin) != T_BIGNUM) {
    rb_raise(ePasoriError, "%s", "Argument error.");
  }

  area = rb_class_new_instance(0, 0, klass);

  rb_iv_set(area, "attr", attr);
  rb_iv_set(area, "code", code);
  rb_iv_set(area, "bin", bin);

  return area;
}

static VALUE
rb_felica_area_get_attr(VALUE self)
{
  return rb_iv_get(self, "attr");
}

static VALUE
rb_felica_area_get_code(VALUE self)
{
  return rb_iv_get(self, "code");
}

static VALUE
rb_felica_area_get_bin(VALUE self)
{
  return rb_iv_get(self, "bin");
}


static VALUE
rb_felica_area_is_protected(VALUE self)
{
  int attr;
  VALUE r;

  attr = NUM2INT(rb_iv_get(self, "attr"));

  if (attr & 1)
    r = Qfalse;
  else
    r = Qtrue;

  return r;
}


void 
Init_felica_area(void) {
  VALUE rb_cFelicaArea;

  rb_cFelicaArea = rb_define_class("FelicaArea", rb_cObject);

  rb_define_singleton_method(rb_cFelicaArea, "new", rb_felica_area_new, 3);

  rb_define_method(rb_cFelicaArea, "attr", rb_felica_area_get_attr, 0);
  rb_define_method(rb_cFelicaArea, "code", rb_felica_area_get_code, 0);
  rb_define_method(rb_cFelicaArea, "bin",  rb_felica_area_get_bin, 0);
  rb_define_method(rb_cFelicaArea, "to_i",  rb_felica_area_get_bin, 0);
  rb_define_method(rb_cFelicaArea, "protected?",  rb_felica_area_is_protected, 0);
}
