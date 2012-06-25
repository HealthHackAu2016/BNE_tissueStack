package au.edu.uq.cai.TissueStack.resources;

import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;

import au.edu.uq.cai.TissueStack.dataobjects.Response;
import au.edu.uq.cai.TissueStack.rest.AbstractRestfulMetaInformation;

public class RestfulResource extends AbstractRestfulMetaInformation {
	private Response response;

	public RestfulResource() {}

	public RestfulResource(Response dataSource) {
		this.response = dataSource;
	}
	
	public Response getReponse() {
		return this.response;
	}

	public void setReponse(Response response) {
		this.response = response;
	}

	@GET
    public RestfulResource getResource() {
		return this;
	}

	@GET
	@Path("/")
	@Produces(MediaType.APPLICATION_XML)
    public Object getDefault() {
		return this.getXML();
	}

	@GET
	@Path("/xml")
	@Produces(MediaType.APPLICATION_XML)
    public Object getXML() {
		if (this.response.hasErrorOccured()) { 
			return this.response.getError();
		}
		return this.response.getResponse();
	}

	@GET
	@Path("/json")
	@Produces(MediaType.APPLICATION_JSON)
    public Response getJSON() {
		return this.response;
	}

	@POST
	@Path("/")
	@Produces(MediaType.APPLICATION_XML)
    public Object postDefault() {
		return this.getXML();
	}

	@POST
	@Path("/xml")
	@Produces(MediaType.APPLICATION_XML)
    public Object postXML() {
		if (this.response.hasErrorOccured()) { 
			return this.response.getError();
		}
		return this.response.getResponse();
	}

	@POST
	@Path("/json")
	@Produces(MediaType.APPLICATION_JSON)
    public Response postJSON() {
		return this.response;
	}
	
	@GET
	@Path("/text")
	@Produces(MediaType.TEXT_PLAIN)
    public String getPlainText() {
		return (String) this.response.getResponse().toString();
	}
}

