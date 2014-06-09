$( document ).ready(function() {

	$('#top > div > ul > li > a').wrapInner("<b></b>");

	$("div.header").prepend("<h2></h2>");
	$("div.header > h2").text($("div.title").text());
	$("div.headertitle").remove();
	$("#nav-path").remove();
	
	$("body").wrapInner("<div class='container'><div class='row'><div class='col-md-12'></div></div></div>");
	$("ul.tablist").addClass("nav nav-pills nav-justified");
	$("ul.tablist").css("margin-top", "0.5em");
	$("ul.tablist").css("margin-bottom", "0.5em");
	$("li.current").addClass("active");
	$(".contents").wrapInner("<div class='panel panel-default' style='margin-top:1em;'><div class='panel-body'></div></div>");
	$(".contents").find("h2.groupheader:first").remove();

	$("iframe").attr("scrolling", "yes");
	
	$('img[src="ftv2ns.png"]').replaceWith('<span class="label label-danger">N</span> ');
	$('img[src="ftv2cl.png"]').replaceWith('<span class="label label-danger">C</span> ');

	// dirty workaround since doxygen 1.7 doesn't set table ids
	$("table").each(function() {
		if($(this).find("tbody > tr > td.indexkey").length > 0) {
			$(this).addClass("table table-striped");
		}
	});
	
	$("table.params").addClass("table");
	$("div.ingroups").wrapInner("<small></small>");
	$("div.levels").css("margin", "0.5em");
	$("div.levels > span").addClass("btn btn-default btn-xs");
	$("div.levels > span").css("margin-right", "0.25em");
	
	$("table.directory").addClass("table table-striped");
	$("div.summary > a").addClass("btn btn-default btn-xs");
	$("table.fieldtable").addClass("table");
	$(".fragment").addClass("well");
	$(".memitem").addClass("panel panel-default");
	$(".memproto").addClass("panel-heading");
	$(".memdoc").addClass("panel-body");
	$("span.mlabel").addClass("label label-info");
	
	$("table.memberdecls").addClass("table");
	$("[class^=memitem]").addClass("active");
	$("td.memSeparator").remove()
	
	$("div.ah").addClass("btn btn-default");
	$("span.mlabels").addClass("pull-right");
	$("table.mlabels").css("width", "100%")
	$("td.mlabels-right").addClass("pull-right");

	$("div.ttc").addClass("panel panel-info");
	$("div.ttname").addClass("panel-heading");
	$("div.ttdef,div.ttdoc,div.ttdeci").addClass("panel-body");
});

