(function(jQuery) {
    jQuery.fn.extend({
        collapsiblePanel: function() {
            // Call the ConfigureCollapsiblePanel function for the selected element
            return jQuery(this).each(ConfigureCollapsiblePanel);
        }
    });

})(jQuery);

function ConfigureCollapsiblePanel() {
    jQuery(this).addClass("ui-widget");

    // Check if there are any child elements, if not then wrap the inner text within a new div.
    if (jQuery(this).children().length == 0) {
        jQuery(this).wrapInner("<div></div>");
    }

    // Wrap the contents of the container within a new div.
    jQuery(this).children().wrapAll("<div class='collapsibleContainerContent ui-widget-content'></div>");

    // Create a new div as the first item within the container.  Put the title of the panel in here.
    jQuery("<div class='collapsibleContainerTitle ui-widget-header'><div>" + jQuery(this).attr("title") + "</div></div>").prependTo(jQuery(this));

    // Assign a call to CollapsibleContainerTitleOnClick for the click event of the new title div.
    jQuery(".collapsibleContainerTitle", this).click(CollapsibleContainerTitleOnClick);

	// Collapse by default
    jQuery(".collapsibleContainerContent").hide();
}

function CollapsibleContainerTitleOnClick() {
    // The item clicked is the title div... get this parent (the overall container) and toggle the content within it.
    jQuery(".collapsibleContainerContent", jQuery(this).parent()).slideToggle();
}
